//---------------------------------------------------------------------------

#pragma hdrstop

#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>
#include <winsock.h>
#include <windows.h>
//---------------------------------------------------------------------------

#pragma argsused

#define MSGBUFSIZE 32000

#pragma pack(push, 1)
typedef struct {
	unsigned char imp_freq;
	unsigned char imp_diag;
	unsigned char word_count;
	unsigned char adc_offset;
	unsigned char adc_accum;
	unsigned imp_length:12;
	unsigned adc_delay:12;
} ch_param_struct;
#pragma pack(pop)

#define ppp_int(nn)	printf(#nn" :");		\
	for(int i = 0; i < uss; i++)			\
    	printf("%i ", ch_param[i].##nn);	\
    printf("\r\n")

#define ppp_hex(nn)	printf(#nn" :");			\
	for(int i = 0; i < uss; i++)				\
    	printf("0x%02X ", ch_param[i].##nn);	\
    printf("\r\n")

void decode_packet(unsigned char *buf, int size) {
	unsigned char *ptr = buf;
    unsigned int id = *(unsigned int *)ptr; ptr += sizeof(unsigned int);
    unsigned int pkt_cntr = *(unsigned int *)ptr; ptr += sizeof(unsigned int);
    unsigned int master_frame = *(unsigned int *)ptr; ptr += sizeof(unsigned int);
    unsigned int gtick = *(unsigned int *)ptr; ptr += sizeof(unsigned int);
    unsigned int mg_us_step = *(unsigned int *)ptr; ptr += sizeof(unsigned int);
    int mgs = mg_us_step >> 24;
    int uss = (mg_us_step >> 16) & 0xFF;

    printf("\r\nID=%08X\r\n", id);
    printf("mf=%i pkt_cntr=%i GHz_tick=%i : mg_step=%i us_count=%i\r\n", master_frame,
    	pkt_cntr, gtick, mgs, uss);

    for(int i = 0; i < mgs; i++) {
    	unsigned int mg1 = *(unsigned int *)ptr; ptr += sizeof(unsigned int);
    	unsigned int mg2 = *(unsigned int *)ptr; ptr += sizeof(unsigned int);
        printf("%i %i \r\n", mg1, mg2);
    }

    ch_param_struct ch_param[20];
    for(int i = 0; i < uss; i++) {
    	ch_param[i] = *(ch_param_struct *)ptr;
        ptr += sizeof(ch_param_struct);
    }

    ppp_int(imp_freq);
    ppp_hex(imp_diag);
    ppp_int(word_count);
    ppp_int(adc_offset);
    ppp_int(adc_accum);
    ppp_int(imp_length);
    ppp_int(adc_delay);

    printf("us_channel_mask:");
    unsigned char us_mask[20];
    for(int i = 0; i < uss; i++) {
    	us_mask[i] = *ptr; ptr++;
        printf(" 0x%02X", us_mask[i]);
    }
    printf("\r\n");
}

unsigned int __stdcall thread_proc(void *param)
{
    char hostname[300];
    hostent * hp;
    gethostname(hostname,300);
    struct in_addr * _addr;
    struct in_addr m_interface;

    if ((hp=gethostbyname(hostname))==NULL) {
//      fprintf(stderr,"Can't get host name\n");
      exit(1);
    } else {
      for(int i=0; i<10; i++) {
        _addr=(struct in_addr *)hp->h_addr_list[i];
        if(_addr == NULL) break;
        int netw = _addr->S_un.S_addr & 0xFFFFFF;
        if(netw == 0x01A8C0) { // add ONLY!!! 192.168.1.0 network addr
            m_interface.s_addr=_addr->s_addr;
            break;
        }
      }
    }

    printf("from thread...\r\n");
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    sockaddr_in localif;
 	localif.sin_family = AF_INET;
	localif.sin_port = htons(1457);
	localif.sin_addr.s_addr = htonl(INADDR_ANY);

	int br = bind(sock, (SOCKADDR *)&localif, sizeof(localif));
    printf("bind res=%i", br);

    struct ip_mreq mreq;
    //sockaddr_in imr_interface;

	mreq.imr_interface = m_interface;
	mreq.imr_multiaddr.s_addr = inet_addr("239.255.0.10");
	setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq));

	struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // differs from sender
	addr.sin_port = htons(1457);

    char msgbuf[MSGBUFSIZE];
        int addrlen = sizeof(addr);

    int n = 0;
    while(n++ < 10) {
        int nbytes = recv(
            sock,
            msgbuf,
            MSGBUFSIZE,
            0);
        decode_packet(msgbuf, nbytes);
        if (nbytes < 0) {
            perror("recvfrom");
            return 1;
        }
    }
//    msgbuf[nbytes] = '\0';
//    puts(msgbuf);
}

int _tmain(int argc, _TCHAR* argv[])
{
	printf("start..\r\n");

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2), &wsaData);

    HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &thread_proc, NULL, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);

    WSACleanup();

    getchar();
	return 0;
}
//---------------------------------------------------------------------------
