// Verilator
#include "svdpi.h"
#include "Vexample_if__Dpi.h"

// Ethernet
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

// Note: needs to be run with SUDO
// On a linux machine(e.g. raspberry pi), use the command: tcpdump -xx --interface=eth0  (or whatever the name of the ethenet interface is)
// to see the packet coming in

// Main
typedef struct txn_in {
  unsigned char x[4]; // equivalent to unsigned int
  unsigned char y[4];
} txn_in;

typedef struct txn_out {
  unsigned char z[4];
} txn_out;

void print_txn_in(txn_in txn) { // pass by value, should be fine lol
  printf("[");
  for(int i=0; i < sizeof(txn.x); i++) {
    printf("%02X ", txn.x[i]);
    fflush(stdout);
  }

  printf("| ");
  fflush(stdout);
  for(int i=0; i < sizeof(txn.y); i++) {
    printf("%02X ", txn.y[i]);
  fflush(stdout);
  }
  printf("]\n");
  fflush(stdout);
}

void print_txn_out(txn_out txn) {
  printf("[");
  fflush(stdout);
  for(int i=0; i < sizeof(txn.z); i++) {
    printf("%02X ", txn.z[i]);
    fflush(stdout);
  }
  printf("]\n");
  fflush(stdout);
}

// Queue implementations (very scuffed lmao)
static const int QUEUE_SIZE_MAX=1000;

typedef struct queue_in_t {
  int head = 0;
  int tail = 0;
  txn_in data[QUEUE_SIZE_MAX];
} queue_in_t;

typedef struct queue_out_t {
  int head = 0;
  int tail = 0;
  txn_out data[QUEUE_SIZE_MAX];
} queue_out_t;

queue_in_t  txn_in_queue;
queue_out_t txn_out_queue;

int push_txn_in_queue(txn_in txn) {
  if(txn_in_queue.tail >= QUEUE_SIZE_MAX - 1) return 1; // queue full
  txn_in_queue.data[txn_in_queue.tail++] = txn;
  return 0;
}

int pop_txn_in_queue(txn_in* txn) {
  if(txn_in_queue.head >= txn_in_queue.tail) return 1; // queue empty
  *txn = txn_in_queue.data[txn_in_queue.head++];
  return 0;
}

int push_txn_out_queue(txn_out txn) {
  if(txn_out_queue.tail >= QUEUE_SIZE_MAX - 1) return 1; // queue full
  txn_out_queue.data[txn_out_queue.tail++] = txn;
  return 0;
}

int pop_txn_out_queue(txn_out* txn) {
  if(txn_out_queue.head >= txn_out_queue.tail) return 1; // queue empty
  *txn = txn_out_queue.data[txn_out_queue.head++];
  return 0;
}


typedef struct ethernet_frame_to_dut_t {
  // mandatory ethernet header info
  unsigned char dst_mac[6];
  unsigned char src_mac[6];
  unsigned char eth_type[2];

  // Data, can be anything
  unsigned char dataX[6];
  unsigned char dataY[6];
  unsigned char pad[44];
} ethernet_frame_to_dut_t;

typedef struct ethernet_frame_from_dut_t {
  // mandatory ethernet header info
  unsigned char dst_mac[6];
  unsigned char src_mac[6];
  unsigned char eth_type[2];

  // Data, can be anything
  unsigned char dataZ[12];
  unsigned char pad[44];
} ethernet_frame_from_dut_t;

// Util function(s)
unsigned char* uint2uchar(unsigned int x) {
  unsigned char  byte;
  size_t size_x = sizeof(x);
  unsigned char* byte_arr = (unsigned char*)malloc(size_x);

  for(int i = size_x - 1; i >= 0; i--) {
    byte = (unsigned char) x;
    byte_arr[i] = byte;
    x = x >> 8;
  }
  return byte_arr;
}

static ethernet_frame_from_dut_t buffer;

// dpi functions
void dpi_reset() { 
  printf("[DPI] DUT reset\n"); // just a stub, rpi does not support a "reset"
}

void dpi_init_calc(unsigned int x_in, unsigned int y_in) {
  txn_in txn;
  memcpy(txn.x, uint2uchar(x_in), sizeof(unsigned int));
  memcpy(txn.y, uint2uchar(y_in), sizeof(unsigned int));
  push_txn_in_queue(txn);
}

unsigned int result_r = 0;
unsigned int get_result() {
  printf("[dpi_get_result] %x\n", result_r);
  return result_r;
};

// pool a queue until
// return 0 for success otherwise return 1
unsigned int dpi_wait_rslt() { // return variable r
  // this should probably be it's own thread
  txn_out txn;
  unsigned int r = 0;

  fflush(stdout);
  if(pop_txn_out_queue(&txn)) return 1;

  printf("[dpi_wait_rslt] sizeof(txn.z): %d\n", sizeof(txn.z));
  print_txn_out(txn);

  for(int i = 0; i < sizeof(txn.z) - 1; i++) {
    r = r | txn.z[i];
    r = r << 8;
    printf("[dpi_wait_rslt]  txn.z(%d): %02X\n", i, txn.z[i]);
    printf("[dpi_wait_rslt]  r(%d)    : %X\n", i, r);
  }
  printf("[dpi_wait_rslt] txn (again): ");
  print_txn_out(txn);

  result_r = r;
  printf("[dpi_wait_rslt ] r: %x\n", r);
  fflush(stdout);
  return 0;
}

// invoked by init(), which runs this in a seperate pthread
void* init_1(void*) {
  int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

  struct sockaddr_ll sll;
  sll.sll_family   = AF_PACKET;
  sll.sll_protocol = htons(ETH_P_ALL);
  sll.sll_ifindex  = 2;

  bind(sockfd, 
      (struct sockaddr *) &sll, 
      sizeof(sll)
      );

ethernet_frame_to_dut_t header = {
    {0xDC, 0xA6, 0x32, 0x7C, 0xE1, 0x39},
    {0xF0, 0xDE, 0xF1, 0xB7, 0x83, 0xCF},
    {0x88, 0x88},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00}
};

  ssize_t write_count;
  ssize_t read_count;
  for(int num_pkts = 0;;num_pkts++){
    static struct timespec ts = {
     .tv_sec  = 0,
     .tv_nsec = 1000
    };

    txn_in txn_i; 
    printf("[init_1] restarting loop\n");
    fflush(stdout);

    while(pop_txn_in_queue(&txn_i)) {
      // TODO add call to systemverilog exported task "consume_sim_time()" which just invokes the #0 statement NOTE Can't do this in verilator :0
      // NOTE: above is actually not supported in verilator, i worked around this by running init_1() in seperate pthread
      //consume_sim_time();
      nanosleep(&ts, &ts);
    }

    fflush(stdout);
    printf("\n[init_1] txn_i: ");
    print_txn_in(txn_i);

    memcpy(header.dataX, txn_i.x, sizeof(txn_i.x));
    memcpy(header.dataY, txn_i.y, sizeof(txn_i.y));

    printf("\n[init_1] header.dataX:");
    for(int i=0; i < sizeof(header.dataX); i++) {
      printf("%02X ", header.dataX[i]);
    }

    printf("\n[init_1] header.dataY:");
    for(int i=0; i < sizeof(header.dataY); i++) {
      printf("%02X ", header.dataY[i]);
    }
    printf("\n");
    fflush(stdout);

    write_count = write(
        sockfd,                        // SOCK ID
        (unsigned char *) &header,     // Buffer to send
        sizeof(header)                 // length of sent buffer
        );

    while(1) {
      read_count = read(
          sockfd,                        // SOCK ID
          (unsigned char *) &buffer,     // Buffer to read
          sizeof(buffer)                 // length of said buffer
          );

      if(read_count == -1) continue; // ignore failed requeest
      if(buffer.eth_type[0] != 0x88 || buffer.eth_type[1] != 0x88) continue; // ignore packets with wrong eth_type
                                                                             //
      printf("\nresp(%d):", num_pkts);
      for(int i = 0; i < sizeof(buffer.dataZ); i++) {
        printf("%02X ", buffer.dataZ[i]);
      }
      printf("\n");

      // --------------------------
      // buffer is bigger in size than txn_o potential problem spot later!!!! (but not now >:) )
      txn_out txn_o;
      memcpy(txn_o.z, buffer.dataZ, sizeof(txn_o.z));
      // --------------------------

      push_txn_out_queue(txn_o);

      fflush(stdout);
      break;
    }
  }

  printf("\n");
  return NULL;
}

int init() {
  pthread_t main_thread;
  pthread_create(&main_thread, NULL, init_1, NULL); // just runs forever

  // main thread should probably be split into two, one
  // for polling queue from verilog and the other for
  // polling queue from the ethernet socket i.e. DUT
  // which should be more performant but not necessary
  return 0;
}
