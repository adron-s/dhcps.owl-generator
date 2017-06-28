#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "templates.h"

#define NETWORK_XML_FILE "/app/webroot/WebApp/common/config/lan/config.xml"
#define TEMPLATE_FILE "/system/etc/dhcps.owl.templ"

#define BITS(n) ((unsigned int)(1 << n) - 1)
#define IPMASK(n) (~BITS(32 - n))

#define PRINTIP(ip){							\
	struct in_addr tmp;							\
	tmp.s_addr = htonl(ip);					\
	printf("%s\n", inet_ntoa(tmp));	\
}

static struct templ_var templ_vars[] = {
	{ "ROUTER",  "" },
	{ "SUBNET_MASK", "255.255.255.0" },
	{ "DHCP_START_IP", "" },
	{ "DHCP_END_IP", "" },
};

static struct templ_var network_xml_file_vars[] = {
	{ "ipaddress", "" },
	{ "mask", "" },
	{ "startip", "" },
	{ "endip",  "" },
};

char *ipaddr, *mask, *startip, *endip;

//**********************************************************************************
/* обрабатывает полученные из xml конфига сети переменные */
void invoke_network_vars(){
	struct in_addr tmp;
	uint32_t modem_ip, gw, subnet, ipmask, minip, maxip;
	inet_aton(ipaddr, &tmp); modem_ip = ntohl(tmp.s_addr);
	inet_aton(mask, &tmp); ipmask = ntohl(tmp.s_addr);
	subnet = modem_ip & ipmask;
	minip = subnet + 1; //минимальный ip в сети
	maxip = (subnet | ~ipmask) - 1; //максимальный ip в сети

	//рассчет ведомственного шлюза
	if(ipmask <= IPMASK(24)){
		//для /24 и ниже: 251 - резерв, 252-терминал/модем, 253-vipnet, 254-шлюз: 4 штуки!
		//адреса для клиентов: 1..250
		gw = maxip;
		maxip = gw - 4; //4 штуки^
	}else{
		//для /25 и выше: первый(шлюз)..клиенты..последний(терминал/модем)
		gw = minip;
		minip++;
		maxip--;
	}
	tmp.s_addr = htonl(gw);
	strcpy(ipaddr, inet_ntoa(tmp));

	//проверка и коррекция если нужно startip/endip
	inet_aton(startip, &tmp);
	if(ntohl(tmp.s_addr) < minip){
		tmp.s_addr = htonl(minip);
		strcpy(startip, inet_ntoa(tmp));
	}
	inet_aton(endip, &tmp);
	if(ntohl(tmp.s_addr) > maxip){
		tmp.s_addr = htonl(maxip);
		strcpy(endip, inet_ntoa(tmp));
	}
}//---------------------------------------------------------------------------------

int main(void){
	//грузим конфиг сети в xml формате
	templ_vars_setup(network_xml_file_vars);
	load_templ_vars_vals_from_file(NETWORK_XML_FILE);
	//dump_templ_vars();
	ipaddr = get_templ_var_val("ipaddress");
	mask = get_templ_var_val("mask");
	startip = get_templ_var_val("startip");
	endip = get_templ_var_val("endip");

	//обрабатываем полученные переменные
	/* после ее вызова ipaddr уже будет указывать не на
		 адрес модема а на ведомственный шлюз! */
	invoke_network_vars();

	//перегружаем его в шаблонизатор для результата
	templ_vars_setup(templ_vars);
	set_templ_var_val("ROUTER", ipaddr);
	set_templ_var_val("SUBNET_MASK", mask);
	set_templ_var_val("DHCP_START_IP", startip);
	set_templ_var_val("DHCP_END_IP", endip);
	//dump_templ_vars();

	//парсим и выводим результат
	parse_templ(TEMPLATE_FILE);
}
