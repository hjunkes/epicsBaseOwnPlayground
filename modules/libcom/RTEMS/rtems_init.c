/*************************************************************************\
* Copyright (c) 2002 The University of Saskatchewan
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * RTEMS startup task for EPICS
 *      Author: W. Eric Norum
 *              eric.norum@usask.ca
 *              (306) 966-5394
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <rtems.h>
#include <rtems/malloc.h>
#include <rtems/error.h>
#include <rtems/stackchk.h>
#include <rtems/rtems_bsdnet.h>
#include <rtems/imfs.h>
#include <rtems/shell.h>
#include <librtemsNfs.h>
#if __RTEMS_MAJOR__>4
#include <pthread.h>
#include <signal.h>
#include <sched.h>
#include <rtems/libio.h>
#include <rtems/telnetd.h>
#include <epicsStdio.h>
#include <epicsReadline.h>
#endif

#include <bsp.h>

#include "epicsVersion.h"
#include "epicsThread.h"
#include "epicsTime.h"
#include "epicsExit.h"
#include "envDefs.h"
#include "errlog.h"
#include "logClient.h"
#include "osiUnistd.h"
#include "iocsh.h"
#include "osdTime.h"
#include "epicsMemFs.h"

#include "epicsRtemsInitHooks.h"

#define RTEMS_VERSION_INT  VERSION_INT(__RTEMS_MAJOR__, __RTEMS_MINOR__, 0, 0)

/*
 * Prototypes for some functions not in header files
 */
void tzset(void);
int fileno(FILE *);
int main(int argc, char **argv);

static void
logReset (void)
{
    void rtems_bsp_reset_cause(char *buf, size_t capacity) __attribute__((weak));
    void (*fp)(char *buf, size_t capacity) = rtems_bsp_reset_cause;

    if (fp) {
        char buf[80];
        fp(buf, sizeof buf);
        errlogPrintf ("Startup after %s.\n", buf);
    }
    else {
        errlogPrintf ("Startup.\n");
    }
}

/*
 ***********************************************************************
 *                         FATAL ERROR REPORTING                       *
 ***********************************************************************
 */
/*
 * Delay for a while, then terminate
 */
static void
delayedPanic (const char *msg)
{
#if __RTEMS_MAJOR__>4
    rtems_task_wake_after (rtems_clock_get_ticks_per_second());
    rtems_task_wake_after (rtems_clock_get_ticks_per_second());
#else
    extern rtems_interval rtemsTicksPerSecond;

    rtems_task_wake_after (rtemsTicksPerSecond);
#endif
    rtems_panic (msg);
}

/*
 * Log error and terminate
 */
void
LogFatal (const char *msg, ...)
{
    va_list ap;

    va_start (ap, msg);
    errlogVprintf (msg, ap);
    va_end (ap);
    delayedPanic (msg);
}

/*
 * Log RTEMS error and terminate
 */
void
LogRtemsFatal (const char *msg, rtems_status_code sc)
{
    errlogPrintf ("%s: %s\n", msg, rtems_status_text (sc));
    delayedPanic (msg);
}

/*
 * Log network error and terminate
 */
void
LogNetFatal (const char *msg, int err)
{
    errlogPrintf ("%s: %d\n", msg, err);
    delayedPanic (msg);
}

void *
mustMalloc(int size, const char *msg)
{
    void *p;

    if ((p = malloc (size)) == NULL)
        LogFatal ("Can't allocate space for %s.\n", msg);
    return p;
}

#if __RTEMS_MAJOR__>4
/*
 ***********************************************************************
 *                         TELNET DAEMON                               *
 ***********************************************************************
 */
#define LINE_SIZE 256
static void
telnet_pseudoIocsh(char *name, void *arg)
{
  char line[LINE_SIZE];
  int fid[3], save_fid[3];

  printf("info:  pty dev name = %s\n", name);

  save_fid[1] = dup2(1,1);
  fid[1] = dup2( fileno(stdout), 1);
  if (fid[1] == -1 ) printf("Can't dup stdout\n");
  save_fid[2] = dup2(2,2);
  fid[2] = dup2( fileno(stderr), 2);
  if (fid[2] == -1 ) printf("Can't dup stderr\n");
    
  const char *prompt = "tIocSh> ";

  while (1) {
    fputs(prompt, stdout);
    fflush(stdout);
    /* telnet close not detected ??? tbd */
    if (fgets(line, LINE_SIZE, stdin) == NULL) {
      dup2(save_fid[1],1);
+     dup2(save_fid[2],2);
      return;
    }
    if (line[strlen(line)-1] == '\n') line[strlen(line)-1] = 0;
    if (!strncmp( line, "bye",3)) {
      printf( "%s", "Will end session\n");
      dup2(save_fid[1],1);
      dup2(save_fid[2],2);
      return;                                                                                                     
     }
     iocshCmd(line);
   }
}

#define SHELL_ENTRY telnet_pseudoIocsh

/*
 *  Telnet daemon configuration
 */
rtems_telnetd_config_table rtems_telnetd_config = {
  .command = SHELL_ENTRY,
  .arg = NULL,
  .priority = 99, // if RTEMS_NETWORK and .priority == 0 bsd_network_prio should be used ...
  .stack_size = 0,
  .client_maximum = 5, // should be 1, on RTEMS and Epics it makes only sense for one connection a time
  .login_check = NULL,
  .keep_stdio = false
};

#endif
/*
 ***********************************************************************
 *                         REMOTE FILE ACCESS                          *
 ***********************************************************************
 */
#ifdef OMIT_NFS_SUPPORT
# include <rtems/tftp.h>
#endif

const epicsMemFS *epicsRtemsFSImage __attribute__((weak));
const epicsMemFS *epicsRtemsFSImage = (void*)&epicsRtemsFSImage;

/* hook to allow app specific FS setup */
int
epicsRtemsMountLocalFilesystem(char **argv) __attribute__((weak));
int
epicsRtemsMountLocalFilesystem(char **argv)
{
    if(epicsRtemsFSImage==(void*)&epicsRtemsFSImage)
        return -1; /* no FS image provided. */
    else if(epicsRtemsFSImage==NULL)
        return 0; /* no FS image provided, but none is needed. */
    else {
        printf("***** Using compiled in file data *****\n");
        if (epicsMemFsLoad(epicsRtemsFSImage) != 0) {
            printf("Can't unpack tar filesystem\n");
            return -1;
        } else {
            argv[1] = "/";
            return 0;
        }
    }
}

static int
initialize_local_filesystem(char **argv)
{
    extern char _DownloadLocation[] __attribute__((weak));
    extern char _FlashBase[] __attribute__((weak));
    extern char _FlashSize[]  __attribute__((weak));

    argv[0] = rtems_bsdnet_bootp_boot_file_name;
    if (epicsRtemsMountLocalFilesystem(argv)==0) {
        return 1; /* FS setup successful */
    } else if (_FlashSize && (_DownloadLocation || _FlashBase)) {
        extern char _edata[];
        size_t flashIndex = _edata - _DownloadLocation;
        char *header = _FlashBase + flashIndex;

        if (memcmp(header + 257, "ustar  ", 8) == 0) {
            int fd;
            printf ("***** Unpack in-memory file system (IMFS) *****\n");
            if (rtems_tarfs_load("/", (unsigned char *)header, (size_t)_FlashSize - flashIndex) != 0) {
                printf("Can't unpack tar filesystem\n");
                return 0;
            }
            if ((fd = open(rtems_bsdnet_bootp_cmdline, 0)) >= 0) {
                close(fd);
                printf ("***** Found startup script (%s) in IMFS *****\n", rtems_bsdnet_bootp_cmdline);
                argv[1] = rtems_bsdnet_bootp_cmdline;
                return 1;
            }
            printf ("***** Startup script (%s) not in IMFS *****\n", rtems_bsdnet_bootp_cmdline);
        }
    }
    return 0;
}

#ifndef OMIT_NFS_SUPPORT
#if __RTEMS_MAJOR__>4 || \
   (__RTEMS_MAJOR__==4 && __RTEMS_MINOR__>9) || \
   (__RTEMS_MAJOR__==4 && __RTEMS_MINOR__==9 && __RTEMS_REVISION__==99)
int
nfsMount(char *uidhost, char *path, char *mntpoint)
{
    int   devl = strlen(uidhost) + strlen(path) + 2;
    char *dev;
    int   rval = -1;

    if ((dev = malloc(devl)) == NULL) {
        fprintf(stderr,"nfsMount: out of memory\n");
        return -1;
    }
    sprintf(dev, "%s:%s", uidhost, path);
    printf("Mount %s on %s\n", dev, mntpoint);
#if __RTEMS_MAJOR__>4
   rval = mount_and_make_target_path (
        dev, mntpoint, RTEMS_FILESYSTEM_TYPE_NFS,
        RTEMS_FILESYSTEM_READ_WRITE, NULL );
   if(rval)
      perror("mount failed");
#else
    if (rtems_mkdir(mntpoint, S_IRWXU | S_IRWXG | S_IRWXO))
        printf("Warning -- unable to make directory \"%s\"\n", mntpoint);
    if (mount(dev, mntpoint, RTEMS_FILESYSTEM_TYPE_NFS,
                             RTEMS_FILESYSTEM_READ_WRITE, NULL)) {
        perror("mount failed");
    }
    else {
        rval = 0;
    }
#endif
    free(dev);
    return rval;
}
#define NFS_INIT
#else
#define NFS_INIT    rpcUdpInit(); nfsInit(0,0);
#endif
#endif


static void
initialize_remote_filesystem(char **argv, int hasLocalFilesystem)
{
#ifdef OMIT_NFS_SUPPORT
    printf ("***** Initializing TFTP *****\n");
#if __RTEMS_MAJOR__>4 || \
   (__RTEMS_MAJOR__==4 && __RTEMS_MINOR__>9) || \
   (__RTEMS_MAJOR__==4 && __RTEMS_MINOR__==9 && __RTEMS_REVISION__==99)
    mount_and_make_target_path(NULL,
                               "/TFTP",
                               RTEMS_FILESYSTEM_TYPE_TFTPFS,
                               RTEMS_FILESYSTEM_READ_WRITE,
                               NULL);
#else
    rtems_bsdnet_initialize_tftp_filesystem ();
#endif
    if (!hasLocalFilesystem) {
        char *path;
        int pathsize = 200;
        int l;

        path = mustMalloc(pathsize, "Command path name ");
        strcpy (path, "/TFTP/BOOTP_HOST/epics/");
        l = strlen (path);
        if (gethostname (&path[l], pathsize - l - 10) || (path[l] == '\0'))
        {
            LogFatal ("Can't get host name");
        }
        strcat (path, "/st.cmd");
        argv[1] = path;
    }
#else
    char *server_name;
    char *server_path;
    char *mount_point;
    char *cp;
    int l = 0;

    printf ("***** Initializing NFS *****\n");
    NFS_INIT
    if (env_nfsServer && env_nfsPath && env_nfsMountPoint) {
        server_name = env_nfsServer;
        server_path = env_nfsPath;
        mount_point = env_nfsMountPoint;
        cp = mount_point;
        while ((cp = strchr(cp+1, '/')) != NULL) {
            *cp = '\0';
            if ((mkdir (mount_point, 0755) != 0)
             && (errno != EEXIST))
                LogFatal("Can't create directory \"%s\": %s.\n",
                                                mount_point, strerror(errno));
            *cp = '/';
        }
        argv[1] = rtems_bsdnet_bootp_cmdline;
    }
    else if (hasLocalFilesystem) {
        return;
    }
    else {
        /*
         * Use first component of nvram/bootp command line pathname
         * to set up initial NFS mount.  A "/tftpboot/" is prepended
         * if the pathname does not begin with a '/'.  This allows
         * NFS and TFTP to have a similar view of the remote system.
         */
        if (rtems_bsdnet_bootp_cmdline[0] == '/')
            cp = rtems_bsdnet_bootp_cmdline + 1;
        else
            cp = rtems_bsdnet_bootp_cmdline;
        cp = strchr(cp, '/');
        if ((cp == NULL)
         || ((l = cp - rtems_bsdnet_bootp_cmdline) == 0))
            LogFatal("\"%s\" is not a valid command pathname.\n", rtems_bsdnet_bootp_cmdline);
        cp = mustMalloc(l + 20, "NFS mount paths");
        server_path = cp;
        server_name = rtems_bsdnet_bootp_server_name;
        if (rtems_bsdnet_bootp_cmdline[0] == '/') {
            mount_point = server_path;
            strncpy(mount_point, rtems_bsdnet_bootp_cmdline, l);
            mount_point[l] = '\0';
            argv[1] = rtems_bsdnet_bootp_cmdline;
            /*
             * Its probably common to embed the mount point in the server
             * name so, when this is occurring, dont clobber the mount point
             * by appending the first node from the command path. This allows
             * the mount point to be a different path then the server's mount
             * path.
             *
             * This allows for example a line similar to as follows the DHCP
             * configuration file.
             *
             * server-name "159.233@192.168.0.123:/vol/vol0/bootRTEMS";
             */
            if ( server_name ) {
                const size_t allocSize = strlen ( server_name ) + 2;
                char * const pServerName = mustMalloc( allocSize,
                                                        "NFS mount paths");
                char * const pServerPath = mustMalloc ( allocSize,
                                                        "NFS mount paths");
                const int scanfStatus = sscanf (
                                          server_name,
                                          "%[^:] : / %s",
                                          pServerName,
                                          pServerPath + 1u );
                if ( scanfStatus ==  2 ) {
                    pServerPath[0u]= '/';
                    server_name = pServerName;
                    // is this a general solution?
                    server_path = mount_point = pServerPath;
                }
                else {
                    free ( pServerName );
                    free ( pServerPath );
                }
            }
        }
        else {
            char *abspath = mustMalloc(strlen(rtems_bsdnet_bootp_cmdline)+2,"Absolute command path");
            strcpy(server_path, "/tftpboot/");
            mount_point = server_path + strlen(server_path);
            strncpy(mount_point, rtems_bsdnet_bootp_cmdline, l);
            mount_point[l] = '\0';
            mount_point--;
            strcpy(abspath, "/");
            strcat(abspath, rtems_bsdnet_bootp_cmdline);
            argv[1] = abspath;
        }
    }
    errlogPrintf("nfsMount(\"%s\", \"%s\", \"%s\")\n",
                 server_name, server_path, mount_point);
    nfsMount(server_name, server_path, mount_point);
#endif
}

static
char rtems_etc_hosts[] = "127.0.0.1       localhost\n";

/* If it doesn't already exist, create /etc/hosts with an entry for 'localhost' */
static
void fixup_hosts(void)
{
    FILE *fp;
    int ret;
    struct stat STAT;

    ret=stat("/etc/hosts", &STAT);
    if(ret==0)
    {
        return; /* already exists, assume file */
    } else if(errno!=ENOENT) {
        perror("error: fixup_hosts stat /etc/hosts");
        return;
    }

    ret = mkdir("/etc", 0775);
    if(ret!=0 && errno!=EEXIST)
    {
        perror("error: fixup_hosts create /etc");
        return;
    }

    if((fp=fopen("/etc/hosts", "w"))==NULL)
    {
        perror("error: fixup_hosts create /etc/hosts");
    }

    if(fwrite(rtems_etc_hosts, 1, sizeof(rtems_etc_hosts)-1, fp)!=sizeof(rtems_etc_hosts)-1)
    {
        perror("error: failed to write /etc/hosts");
    }

    fclose(fp);
}

/*
 * Get to the startup script directory
 * The TFTP filesystem requires a trailing '/' on chdir arguments.
 */
static void
set_directory (const char *commandline)
{
    const char *cp;
    char *directoryPath;
    int l;

    cp = strrchr(commandline, '/');
    if (cp == NULL) {
        l = 0;
        cp = "/";
    }
    else {
        l = cp - commandline;
        cp = commandline;
    }
    directoryPath = mustMalloc(l + 2, "Command path directory ");
    strncpy(directoryPath, cp, l);
    directoryPath[l] = '/';
    directoryPath[l+1] = '\0';
    if (chdir (directoryPath) < 0)
        LogFatal ("Can't set initial directory(%s): %s\n", directoryPath, strerror(errno));
    else
        errlogPrintf("chdir(\"%s\")\n", directoryPath);
    free(directoryPath);
}

/*
 ***********************************************************************
 *                         RTEMS/EPICS COMMANDS                        *
 ***********************************************************************
 */

static const iocshArg rtshellArg0 = { "cmd", iocshArgString};
static const iocshArg rtshellArg1 = { "args", iocshArgArgv};
static const iocshArg * rtshellArgs[2] = { &rtshellArg0, &rtshellArg1};
static const iocshFuncDef rtshellFuncDef = { "rt",2, rtshellArgs};
static void rtshellCallFunc(const iocshArgBuf *args)
{
    rtems_shell_cmd_t *cmd = rtems_shell_lookup_cmd(args[0].sval);
    int ret;
    
    if (!cmd) {
        fprintf(stderr, "ERR: No such command\n");
    
    } else {
        fflush(stdout);
        fflush(stderr);
        ret = (*cmd->command)(args[1].aval.ac,args[1].aval.av);
        fflush(stdout);
        fflush(stderr);
        if(ret)
            fprintf(stderr, "ERR: %d\n",ret);
    }
}

/*
 * RTEMS status
 */
static void
rtems_netstat (unsigned int level)
{
    rtems_bsdnet_show_if_stats ();
    rtems_bsdnet_show_mbuf_stats ();
    if (level >= 1) {
        rtems_bsdnet_show_inet_routes ();
    }
    if (level >= 2) {
        rtems_bsdnet_show_ip_stats ();
        rtems_bsdnet_show_icmp_stats ();
        rtems_bsdnet_show_udp_stats ();
        rtems_bsdnet_show_tcp_stats ();
    }
}

static const iocshArg netStatArg0 = { "level",iocshArgInt};
static const iocshArg * const netStatArgs[1] = {&netStatArg0};
static const iocshFuncDef netStatFuncDef = {"netstat",1,netStatArgs};
static void netStatCallFunc(const iocshArgBuf *args)
{
    rtems_netstat(args[0].ival);
}

static const iocshFuncDef heapSpaceFuncDef = {"heapSpace",0,NULL};
static void heapSpaceCallFunc(const iocshArgBuf *args)
{
#if __RTEMS_MAJOR__>4
    Heap_Information_block info;
    double x;

    malloc_info (&info);
    x = info.Stats.size - (unsigned long)
        (info.Stats.lifetime_allocated - info.Stats.lifetime_freed);
#else
    rtems_malloc_statistics_t s;
    double x;

    malloc_get_statistics(&s);
    x = s.space_available - (unsigned long)(s.lifetime_allocated - s.lifetime_freed);
#endif
    if (x >= 1024*1024)
        printf("Heap space: %.1f MB\n", x / (1024 * 1024));
    else
        printf("Heap space: %.1f kB\n", x / 1024);
}

#ifndef OMIT_NFS_SUPPORT
static const iocshArg nfsMountArg0 = { "[uid.gid@]host",iocshArgString};
static const iocshArg nfsMountArg1 = { "server path",iocshArgString};
static const iocshArg nfsMountArg2 = { "mount point",iocshArgString};
static const iocshArg * const nfsMountArgs[3] = {&nfsMountArg0,&nfsMountArg1,
                                                 &nfsMountArg2};
static const iocshFuncDef nfsMountFuncDef = {"nfsMount",3,nfsMountArgs};
static void nfsMountCallFunc(const iocshArgBuf *args)
{
    char *cp = args[2].sval;
    while ((cp = strchr(cp+1, '/')) != NULL) {
        *cp = '\0';
        if ((mkdir (args[2].sval, 0755) != 0) && (errno != EEXIST)) {
            printf("Can't create directory \"%s\": %s.\n",
                                            args[2].sval, strerror(errno));
            return;
        }
        *cp = '/';
    }
    nfsMount(args[0].sval, args[1].sval, args[2].sval);
}
#endif


void zoneset(const char *zone)
{
    if(zone)
        setenv("TZ", zone, 1);
    else
        unsetenv("TZ");
    tzset();
}

static const iocshArg zonesetArg0 = {"zone string", iocshArgString};
static const iocshArg * const zonesetArgs[1] = {&zonesetArg0};
static const iocshFuncDef zonesetFuncDef = {"zoneset",1,zonesetArgs};
static void zonesetCallFunc(const iocshArgBuf *args)
{
    zoneset(args[0].sval);
}


/*
 * Register RTEMS-specific commands
 */
static void iocshRegisterRTEMS (void)
{
    iocshRegister(&netStatFuncDef, netStatCallFunc);
    iocshRegister(&heapSpaceFuncDef, heapSpaceCallFunc);
#ifndef OMIT_NFS_SUPPORT
    iocshRegister(&nfsMountFuncDef, nfsMountCallFunc);
#endif
    iocshRegister(&zonesetFuncDef, &zonesetCallFunc);
    iocshRegister(&rtshellFuncDef, &rtshellCallFunc);
    
#if __RTEMS_MAJOR__<5
    void rtems_shell_initialize_command_set(void);
    rtems_shell_initialize_command_set();
#else
    rtems_shell_init_environment();
#endif
}

/*
 * Set up the console serial line (no handshaking)
 */
static void
initConsole (void)
{
    struct termios t;

    if (tcgetattr (fileno (stdin), &t) < 0) {
        printf ("tcgetattr failed: %s\n", strerror (errno));
        return;
    }
    t.c_iflag &= ~(IXOFF | IXON | IXANY);
    if (tcsetattr (fileno (stdin), TCSANOW, &t) < 0) {
        printf ("tcsetattr failed: %s\n", strerror (errno));
        return;
    }
}

#if __RTEMS_MAJOR__<5
/*
 * Ensure that the configuration object files
 * get pulled in from the library
 */
extern rtems_configuration_table Configuration;
extern struct rtems_bsdnet_config rtems_bsdnet_config;
const void *rtemsConfigArray[] = {
    &Configuration,
    &rtems_bsdnet_config
};
#endif
/*
 * Hook to ensure that BSP cleanup code gets run on exit
 */
static void
exitHandler(void)
{
    rtems_shutdown_executive(0);
}

/*
 * RTEMS Startup task
 */
#if __RTEMS_MAJOR__>4
void *
POSIX_Init (void *argument)
#else
rtems_task
Init (rtems_task_argument ignored)
#endif
  {
    int                 result;
    char               *argv[3]         = { NULL, NULL, NULL };
    char               *cp;
#if __RTEMS_MAJOR__<5
    rtems_task_priority newpri;
#endif
    rtems_status_code   sc;
    rtems_time_of_day   now;

    /*
     * Explain why we're here
     */
    logReset();

    /*
     * Architecture-specific hooks
     */
    if (epicsRtemsInitPreSetBootConfigFromNVRAM(&rtems_bsdnet_config) != 0)
        delayedPanic("epicsRtemsInitPreSetBootConfigFromNVRAM");
    if (rtems_bsdnet_config.bootp == NULL) {
        extern void setBootConfigFromNVRAM(void);
        setBootConfigFromNVRAM();
    }
    if (epicsRtemsInitPostSetBootConfigFromNVRAM(&rtems_bsdnet_config) != 0)
        delayedPanic("epicsRtemsInitPostSetBootConfigFromNVRAM");

#if __RTEMS_MAJOR__>4

    /*
     * Override RTEMS Posix configuration, starts with posix prio 2
     */
    epicsThreadSetPriority(epicsThreadGetIdSelf(), epicsThreadPriorityIocsh);
#else
     /*
     * Override RTEMS configuration
     */
    rtems_task_set_priority (
      RTEMS_SELF,
      epicsThreadGetOssPriorityValue(epicsThreadPriorityIocsh),
      &newpri);
#endif

    /*
     * Create a reasonable environment
     */
    initConsole ();
    putenv ("TERM=xterm");
    putenv ("IOCSH_HISTSIZE=20");

    /*
     * Display some OS information
     */
    printf("\n***** RTEMS Version: %s *****\n",
        rtems_get_version_string());

    /*
     * Start network
     */
    if ((cp = getenv("EPICS_TS_NTP_INET")) != NULL)
        rtems_bsdnet_config.ntp_server[0] = cp;
    if (rtems_bsdnet_config.network_task_priority == 0)
    {
        unsigned int p;
        if (epicsThreadHighestPriorityLevelBelow(epicsThreadPriorityScanLow, &p)
                                            == epicsThreadBooleanStatusSuccess)
        {
#if __RTEMS_MAJOR__>4
            rtems_bsdnet_config.network_task_priority = 255 - p; //it's the RTEMS prio, check check ...
#else
            rtems_bsdnet_config.network_task_priority = epicsThreadGetOssPriorityValue(p);
#endif
        }
    }
    printf("\n***** Initializing network (network_task_priority = %u) *****\n", rtems_bsdnet_config.network_task_priority );
    rtems_bsdnet_initialize_network();
    printf("\n***** Setting up file system *****\n");
    initialize_remote_filesystem(argv, initialize_local_filesystem(argv));
    fixup_hosts();

    /*
     * More environment: iocsh prompt and hostname
     */
    {
        char hostname[1024];
        gethostname(hostname, 1023);
        char *cp = mustMalloc(strlen(hostname)+3, "iocsh prompt");
        sprintf(cp, "%s> ", hostname);
        epicsEnvSet ("IOCSH_PS1", cp);
        epicsEnvSet("IOC_NAME", hostname);
    }

    /*
     * Use BSP-supplied time of day if available otherwise supply default time.
     * It is very likely that other time synchronization facilities in EPICS
     * will soon override this value.
     */
#if __RTEMS_MAJOR__>4
    if (rtems_clock_get_tod(&now) != RTEMS_SUCCESSFUL) {
#else
    if (rtems_clock_get(RTEMS_CLOCK_GET_TOD,&now) != RTEMS_SUCCESSFUL) {
#endif
        now.year = 2001;
        now.month = 1;
        now.day = 1;
        now.hour = 0;
        now.minute = 0;
        now.second = 0;
        now.ticks = 0;
        if ((sc = rtems_clock_set (&now)) != RTEMS_SUCCESSFUL)
            printf ("***** Can't set time: %s\n", rtems_status_text (sc));
    }
    if (getenv("TZ") == NULL) {
        const char *tzp = envGetConfigParamPtr(&EPICS_TIMEZONE);
        if (tzp == NULL) {
            printf("Warning -- no timezone information available -- times will be displayed as GMT.\n");
        }
        else {
            char tz[10];
            int minWest, toDst = 0, fromDst = 0;
            if(sscanf(tzp, "%9[^:]::%d:%d:%d", tz, &minWest, &toDst, &fromDst) < 2) {
                printf("Warning: EPICS_TIMEZONE (%s) unrecognizable -- times will be displayed as GMT.\n", tzp);
            }
            else {
                char posixTzBuf[40];
                char *p = posixTzBuf;
                p += sprintf(p, "%cST%d:%.2d", tz[0], minWest/60, minWest%60);
                if (toDst != fromDst)
                    p += sprintf(p, "%cDT", tz[0]);
                epicsEnvSet("TZ", posixTzBuf);
            }
        }
    }
    tzset();
#if __RTEMS_MAJOR__>4
    printf(" check for time registered , C++ initialization ...\n");
    // timeRegister();
#else
    osdTimeRegister();
#endif

#if __RTEMS_MAJOR__>4
    // if telnetd is requested ...
    printf(" Will try to start telnetd with prio %d ...\n", rtems_telnetd_config.priority);
    result = rtems_telnetd_initialize();
    printf (" telnetd initialized with result %d\n", result);
#endif

    /*
     * Run the EPICS startup script
     */
    printf("stdin: fileno: %d, ttyname: %s\n", fileno(stdin),ttyname(fileno(stdin)));
    printf("stdout: fileno: %d, ttyname: %s\n", fileno(stdout),ttyname(fileno(stdout)));
    printf("stderr: fileno: %d, ttyname: %s\n", fileno(stderr),ttyname(fileno(stderr)));
    printf ("***** Preparing EPICS application *****\n");

    iocshRegisterRTEMS ();
    set_directory (argv[1]);
    epicsEnvSet ("IOC_STARTUP_SCRIPT", argv[1]);
    atexit(exitHandler);
    errlogFlush();
    printf ("***** Starting EPICS application *****\n");
    result = main ((sizeof argv / sizeof argv[0]) - 1, argv);
    printf ("***** IOC application terminating *****\n");
    epicsThreadSleep(1.0);
    epicsExit(result);
#if __RTEMS_MAJOR__>4
    return NULL;
#endif
}

#if defined(QEMU_FIXUPS)
/* Override some hooks (weak symbols)
 * if BSP defaults aren't configured for running tests.
 */


/* Ensure that stdio goes to serial (so it can be captured) */
#if defined(__i386__) && !USE_COM1_AS_CONSOLE
#include <uart.h>
#if __RTEMS_MAJOR__>4
#include <libchip/serial.h>
#endif
extern int BSPPrintkPort;
void bsp_predriver_hook(void)
{
#if __RTEMS_MAJOR__>4
    Console_Port_Minor = BSP_CONSOLE_PORT_COM1;
#else
    BSPConsolePort = BSP_CONSOLE_PORT_COM1;
#endif
    BSPPrintkPort = BSP_CONSOLE_PORT_COM1;
}
#endif

/* reboot immediately when done. */
#if defined(__i386__) && BSP_PRESS_KEY_FOR_RESET
void bsp_cleanup(void)
{
#if RTEMS_VERSION_INT>=VERSION_INT(4,10,0,0)
    void bsp_reset();
    bsp_reset();
#else
    rtemsReboot();
#endif
}
#endif

#endif /* QEMU_FIXUPS */

int cexpdebug __attribute__((weak));

#define CONFIGURE_SHELL_COMMANDS_INIT
#define CONFIGURE_SHELL_COMMANDS_ALL
// exclude commands which won't work right with our configuration
//#define CONFIGURE_SHELL_NO_COMMAND_EXIT
//#define CONFIGURE_SHELL_NO_COMMAND_LOGOFF
// exclude commands which unnecessarily pull in block driver
#define CONFIGURE_SHELL_NO_COMMAND_MSDOSFMT
#define CONFIGURE_SHELL_NO_COMMAND_BLKSYNC
#define CONFIGURE_SHELL_NO_COMMAND_MKRFS
#define CONFIGURE_SHELL_NO_COMMAND_DEBUGRFS
#define CONFIGURE_SHELL_NO_COMMAND_FDISK

#include <rtems/shellconfig.h>
