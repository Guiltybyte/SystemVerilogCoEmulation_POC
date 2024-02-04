#include <sys/socket.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <linux/if_packet.h>
#include <linux/if_arp.h>
#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // memcpy

// Note: needs to be run with SUDO
// shoud see a netid p_raw, which receives data due to using ETH_P_ALL
// On a linux machine(e.g. raspberry pi), use the command: tcpdump -xx --interface=eth0  (or whatever the name of the ethenet interface is)
// to see the packet coming in

//---------------------
// utility function(s)
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
//---------------------

//---------------------
// DUT mocking functions
void dut_reset() {
  printf("[DUT] DUT reset!\n");
}

unsigned int dut_calc(unsigned int x, unsigned int y) {
  printf("[DUT] initiating calculation\n");
  return x + y;
}	
//---------------------

//---------------------
// Main (Handling raw sockets, parsing)

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
  unsigned char eth_type[2]; // my custom type is 0x8888

  // Data, can be anything, maybe first octet should signal if it is return type or not
  unsigned char dataZ[12]; 
  unsigned char pad[44];
} ethernet_frame_from_dut_t;

int main() {
  int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

  struct sockaddr_ll sll;
  sll.sll_family = AF_PACKET;
  sll.sll_protocol = htons(ETH_P_ALL);
  sll.sll_ifindex = 2;

  bind(sockfd, 
      (struct sockaddr *) &sll, 
      sizeof(sll)
      );

   ethernet_frame_from_dut_t response_buffer = {
    {0xF0, 0xDE, 0xF1, 0xB7, 0x83, 0xCF},
    {0xDC, 0xA6, 0x32, 0x7C, 0xE1, 0x39},
    {0x88, 0x88}, // Custom Transport protcol EtherType value
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00}
  };



  printf("entering read loop\n");
  fflush(stdout);

  unsigned int z;
  unsigned int x;
  unsigned int y;
  ssize_t read_count;
  ssize_t write_count;

  static ethernet_frame_to_dut_t buffer;
  for(int num_pkts = 0;;num_pkts++) {           // do it forever
    while(1) {
      read_count = read(
          sockfd,                        // SOCK ID
          (unsigned char*) &buffer,      // Buffer to receive into
          sizeof(buffer)                 // length of said buffer
          );

      if(read_count == -1) continue;      // ignore failed request
      if(buffer.eth_type[0] != 0x88 || buffer.eth_type[1] != 0x88) continue; // ignore packets with the wrong protocol

      printf("\nrequest(%d)\ndataX: ", num_pkts);
      for(int i = 0; i < sizeof(buffer.dataX); i++) {
        printf("%02X ", buffer.dataX[i]);
	// parse uchar into uint
	x = x | buffer.dataX[i];
        x = x << 8;
      }
      printf("\ndataY: ");
      for(int i = 0; i < sizeof(buffer.dataY); i++) {
        printf("%02X ", buffer.dataY[i]);
	// parse uchar into uint
	y = y | buffer.dataY[i];
        y = y << 8;
      }
      fflush(stdout);
      break; // finish program
      printf("Received data, sending response\n");
      fflush(stdout);
    }

    z = dut_calc(x, y);
    memcpy(response_buffer.dataZ, uint2uchar(z), sizeof(unsigned int));

    write_count = write(
        sockfd,                             // SOCK ID
        (unsigned char *) &response_buffer, // Buffer to send
        sizeof(response_buffer)             // length of said buffer
        );
    printf("Wrote %d packets to socket\n", write_count);
    fflush(stdout);
  }
  return 0;
}
//---------------------
