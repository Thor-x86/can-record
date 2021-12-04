#define _POSIX_C_SOURCE 199309L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <string.h>
#include <time.h>

static const char usageMessage[] = "Usage:\n    %s <interface> [bitrate] <seconds> <path-to-output-file>\n\n";
static const char exampleMessage[] = "Example:\n    %s vcan0 30 testing.csv\n    %s can0 250000 30 /mnt/sdcard/recorded.csv\n\n";
static const char headRow[] = "Timestamp (μs),CAN ID,Data Size (byte),1,2,3,4,5,6,7,8\r\n";
static const char printFormat[] = "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\r\n";

char *canInterface = NULL;
int duration = 0;
char stopCanCmd[30];

void *timerHandler(void *);
unsigned long long getMicrosecond();

int main(int arg_count, char **args)
{
  char *targetPath = NULL;
  char *bitrate = 0;
  if (arg_count < 1)
  {
    const char defaultExec[] = "can-record";
    printf(usageMessage, defaultExec);
    printf(exampleMessage, defaultExec, defaultExec);
  }
  else if (arg_count < 4)
  {
    printf(usageMessage, args[0]);
    printf(exampleMessage, args[0], args[0]);
  }
  else if (arg_count < 5)
  {
    canInterface = args[1];
    duration = atoi(args[2]);
    targetPath = args[3];
  }
  else
  {
    canInterface = args[1];
    bitrate = args[2];
    duration = atoi(args[3]);
    targetPath = args[4];
  }

  if (canInterface == NULL || duration <= 0 || targetPath == NULL)
    return EINVAL;

  if (access(targetPath, F_OK) == 0)
  {
    fprintf(stderr, "\nERROR: \"%s\" file exists, delete it first!\n\n", targetPath);
    return EEXIST;
  }

  sprintf(stopCanCmd, "sudo ip link set %.7s down", canInterface);

  printf("Resetting %s...\n", canInterface);
  system(stopCanCmd);

  if (bitrate != NULL)
  {
    int bitrateInt = atoi(bitrate);
    if (bitrateInt >= 10 && bitrateInt <= 1000000)
    {
      printf("Configuring bitrate %s to %s...\n", canInterface, bitrate);
      char setBitrateCmd[50];
      sprintf(setBitrateCmd, "sudo ip link set %.7s type can bitrate %s", canInterface, bitrate);
      int returnCode = system(setBitrateCmd);
      if (returnCode != EXIT_SUCCESS)
      {
        return returnCode;
      }
    }
    else
    {
      fprintf(stderr, "\nERROR: Valid bitrate is a number between 10 and 1000000\n\n");
      return EINVAL;
    }
  }

  printf("Starting %s...\n", canInterface);
  {
    char startCanCmd[28];
    sprintf(startCanCmd, "sudo ip link set %.7s up", canInterface);
    int returnCode = system(startCanCmd);
    if (returnCode != EXIT_SUCCESS)
    {
      return returnCode;
    }
  }

  printf("Recording to \"%s\" for %u seconds...\n", targetPath, duration);

  int targetFile = creat(targetPath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
  if (targetFile <= 0)
  {
    perror("Failed to create and open the file");
    return EIO;
  }

  int canSocket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (canSocket < 0)
  {
    perror("Failed to open SocketCAN");
    return EIO;
  }

  struct sockaddr_can address;
  struct ifreq ifr;
  strcpy(ifr.ifr_name, canInterface);
  ioctl(canSocket, SIOCGIFINDEX, &ifr);

  memset(&address, 0, sizeof(address));
  address.can_family = AF_CAN;
  address.can_ifindex = ifr.ifr_ifindex;

  int returnCode = bind(canSocket, (struct sockaddr *)&address, sizeof(address));
  if (returnCode < 0)
  {
    perror("Failed to bind SocketCAN");
    return EIO;
  }

  pthread_t timerThread;
  pthread_create(&timerThread, NULL, &timerHandler, NULL);

  write(targetFile, headRow, sizeof(headRow) - 1);

  unsigned long long firstTimestamp = getMicrosecond();
  struct can_frame frame;
  while (1)
  {
    read(canSocket, &frame, sizeof(struct can_frame));
    unsigned long long timestamp = getMicrosecond() - firstTimestamp;
    dprintf(
        targetFile, printFormat,
        timestamp, frame.can_id, frame.can_dlc,
        frame.data[0], frame.data[1], frame.data[2], frame.data[3],
        frame.data[4], frame.data[5], frame.data[6], frame.data[7]);
    printf(
        printFormat,
        timestamp, frame.can_id, frame.can_dlc,
        frame.data[0], frame.data[1], frame.data[2], frame.data[3],
        frame.data[4], frame.data[5], frame.data[6], frame.data[7]);
  }

  pthread_cancel(timerThread);
  system(stopCanCmd);
  return EXIT_FAILURE;
}

void *timerHandler(void *param)
{
  sleep(duration);
  printf("Done!\n");
  printf("Stopping %s...\n", canInterface);
  system(stopCanCmd);
  exit(EXIT_SUCCESS);
  return NULL;
}

unsigned long long getMicrosecond()
{
  struct timespec now = {0, 0};
  clock_gettime(CLOCK_REALTIME, &now);
  return ((now.tv_sec * 1000000ULL) + (now.tv_nsec / 1000ULL));
}