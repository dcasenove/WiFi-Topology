#include "RadiotapScanner.h"
#include <time.h>
using namespace std;

uint32_t crc32(uint32_t bytes_sz, const uint8_t *bytes)
{
   uint32_t crc = ~0;
   uint32_t i;
   for(i = 0; i < bytes_sz; ++i) {
      crc = crctable[(crc ^ bytes[i]) & 0xff] ^ (crc >> 8);
   }
   return ~crc;
}

void RadiotapScanner::findGloballyAdministeredInterface(std::string mac){
  std::cout << "Dentro find" << std::endl;
  auto search = devices.find(mac);
  //devices.erase(search);
  std::string last_three_octects=mac.substr(9,8);
  for(auto n : devices){
    std::cout << "Dentro for find" << std::endl;
    if(n.first.compare(mac)!=0){
      std::cout << "ultimi 3 ottetti" << last_three_octects << std::endl;
      std::size_t s = n.first.find(last_three_octects,9);
      if((s!=std::string::npos)/*&&(n.first.compare(mac)!=0)*/){
        std::cout << "Dentro if" << std::endl;
        n.second->local_assigned_interfaces.push_back(search->second);
        search->second->main_device=n.second;
        return;
      }
    }
  }
  for(auto n : arp){
    std::cout << "Cerco nei risultati arp scan" << std::endl;
    if(n.compare(mac)!=0){
      std::cout << "ultimi 3 ottetti" << last_three_octects << std::endl;
      std::size_t s = n.find(last_three_octects,9);
      if((s!=std::string::npos)){
        std::cout << "Dentro if controllo arp" << std::endl;
        auto search2 = devices.find(n);
        if(search2==devices.end()){
          Device *d = new Device(n);
          devices.insert({n,d});
        }
        std::cout << "Dopo if creazione" << std::endl;
        search2 = devices.find(n);
        search2->second->local_assigned_interfaces.push_back(search->second);
        search->second->main_device=search2->second;
      }
    }
  }
  //Risultati arp scan
}

  void RadiotapScanner::findMainMACAP(std::string mac){
    std::string first_five_octects=mac.substr(0,15);
    std::cout << "Primi cinque ottetti" << first_five_octects << std::endl;
    vector<std::string> found;
    found.push_back(mac);
    for(auto n : devices){
      std::size_t s = n.first.find(first_five_octects,0);
      if((s!=std::string::npos)){
        found.push_back(n.first);
      }
    }
    for(auto n : arp){
      std::size_t s = n.find(first_five_octects,0);
      if((s!=std::string::npos)){
        auto search = devices.find(n);
        if(search==devices.end()){
          Device *d = new Device(n);
          devices.insert({n,d});
          found.push_back(n);
        }
      }
    }
    vector<unsigned> min_vector;
    for(auto str : found){
      std::cout << "Stringa : " <<  "size : " << str.size() << std::endl;
      std::string octect6 = str.substr(str.size()-2,2);
      std::cout << "Ultimo ottetto : " << octect6 << std::endl;
      std::stringstream ss;
      ss << std::hex << octect6;
      unsigned np;
      ss >> np;
      std::cout << "NP:" << np << std::endl;
      min_vector.push_back(np);
    }
    int min=0;
    for(unsigned long i=0; i<min_vector.size(); i++){
      if(min_vector[i]< min_vector[min]){
        min=i;
      }
    }
    auto search = devices.find(found[min]);
    std::cout << "Mac : " << search->first << std::endl;
    auto toinclude = devices.find(mac);
    toinclude->second->main_device=search->second;
    //Controllare assegnamento successivo
    search->second->local_assigned_interfaces.push_back(toinclude->second);
  }



template<typename TInputIter>
std::string make_hex_string(TInputIter first, TInputIter last, bool use_uppercase = true, bool insert_spaces = false)
{
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    if (use_uppercase)
        ss << std::uppercase;
    while (first != last)
    {
        ss << std::setw(2) << static_cast<int>(*first++);
        if (insert_spaces && first != last)
            ss << ":";
    }
    return ss.str();
}

bool isValidMAC(std::string mac_address){
    std::string multicast("01:00:5e");
    std::string broadcast("ff:ff:ff:ff:ff:ff");
    std::string ipv6multicast("33:33:00:00:00");
    std::string ipv6multicast2("33:33:ff");
    std::string cdp("01:00:0c:cc:cc:cc");

    std::size_t found;
    found = mac_address.find(multicast);
    if(found!=std::string::npos){
        return false;
    }
    found = mac_address.find(broadcast);
    if(found!=std::string::npos){
        return false;
    }
    found = mac_address.find(ipv6multicast);
    if(found!=std::string::npos){
        return false;
    }
    found = mac_address.find(ipv6multicast2);
    if(found!=std::string::npos){
        return false;
    }
    found = mac_address.find(cdp);
    if(found!=std::string::npos){
        return false;
    }

    return true;
}

int ieee80211_mhz_to_chan(unsigned int freq) {
    unsigned int i;

    for (i = 0; i < NUM_FREQ_CVT; i++) {
        if (freq >= freq_cvt[i].fmin && freq <= freq_cvt[i].fmax) {
            return ((freq - freq_cvt[i].fmin) / FREQ_STEP) + freq_cvt[i].cmin;
        }
    }
    return -1;
}

static void print_radiotap_namespace(struct ieee80211_radiotap_iterator *iter,struct signal_power *power){
	switch (iter->this_arg_index) {
	case IEEE80211_RADIOTAP_TSFT:
		printf("\tTSFT : %llu\n", le64toh(*(unsigned long long *)iter->this_arg));
		break;
	case IEEE80211_RADIOTAP_FLAGS:
		printf("\tFlags : %02x\n", *iter->this_arg);
		break;
	case IEEE80211_RADIOTAP_RATE:
		printf("\tRate : %lf\n", (double)*iter->this_arg/2);
		break;
	case IEEE80211_RADIOTAP_CHANNEL:
        //Funziona, vedere little/big endian
        printf("\tChannel : %d\n",ieee80211_mhz_to_chan(le16toh(*(unsigned int *) iter->this_arg)));
        printf("\tChannel Frequency : %u\n", le16toh(*(unsigned int *)iter->this_arg));
        power->channel=ieee80211_mhz_to_chan(le16toh(*(unsigned int *) iter->this_arg));
        break;
	case IEEE80211_RADIOTAP_FHSS:
	case IEEE80211_RADIOTAP_DBM_ANTSIGNAL:
  //Qui
        power->antenna_signal=*iter->this_arg;
        printf("\tSignal : %d\n",(signed char) *iter->this_arg);
        break;

	case IEEE80211_RADIOTAP_DBM_ANTNOISE:
        power->antenna_noise=*iter->this_arg;
        printf("\tNoise : %d\n", (signed char)*iter->this_arg);
        break;

	case IEEE80211_RADIOTAP_LOCK_QUALITY:
	case IEEE80211_RADIOTAP_TX_ATTENUATION:
	case IEEE80211_RADIOTAP_DB_TX_ATTENUATION:
	case IEEE80211_RADIOTAP_DBM_TX_POWER:
	case IEEE80211_RADIOTAP_ANTENNA:
        printf("\tAntenna : %d\n",(signed char) *iter->this_arg);
        break;

	case IEEE80211_RADIOTAP_DB_ANTSIGNAL:
        printf("\tSignal : %d\n",(signed char) *iter->this_arg);
        break;
	case IEEE80211_RADIOTAP_DB_ANTNOISE:
        printf("\tNoise : %d\n", (signed char)*iter->this_arg);
        break;
	case IEEE80211_RADIOTAP_TX_FLAGS:
		break;
	case IEEE80211_RADIOTAP_RX_FLAGS:
		if (fcshdr) {
			printf("\tFCS in header : %.8x\n",
				le32toh(*(uint32_t *)iter->this_arg));
			break;
		}
		printf("\tRX flags : %#.4x\n",
			le16toh(*(uint16_t *)iter->this_arg));
		break;
	case IEEE80211_RADIOTAP_RTS_RETRIES:
	case IEEE80211_RADIOTAP_DATA_RETRIES:
		break;
	default:
		printf("\tBOGUS DATA\n");
		break;
	}
}

void dissectpacket(u_char *args, const struct pcap_pkthdr *header,const u_char *packet){
    struct ieee80211_radiotap_iterator iter;
    struct signal_power power;
    static int count = 1;
    int i;
    cout << ("Pacchetto : ") << count << endl;
    printf("%ld.%06d\n", header->ts.tv_sec, header->ts.tv_usec);
    cout << ("Lunghezza header : ") << (int)header->len << endl;
    count++;

    int err;


    err=ieee80211_radiotap_iterator_init(&iter, (ieee80211_radiotap_header*)packet, header->len, &vns);
    if(err){
        cout << ("Malformed header") << endl;
        return;
    }
    cout << ("Init valido") << endl;
    printf("===RADIOTAP HEADER===\n");
    	while (!(err = ieee80211_radiotap_iterator_next(&iter))) {
		if (iter.this_arg_index == IEEE80211_RADIOTAP_VENDOR_NAMESPACE) {
			printf("\tvendor NS (%.2x-%.2x-%.2x:%d, %d bytes)\n",
				iter.this_arg[0], iter.this_arg[1],
				iter.this_arg[2], iter.this_arg[3],
				iter.this_arg_size - 6);
			for (i = 6; i < iter.this_arg_size; i++) {
				if (i % 8 == 6)
					printf("\t\t");
				else
					printf(" ");
				printf("%.2x", iter.this_arg[i]);
			}
			printf("\n");
		} else if (iter.is_radiotap_ns)
			print_radiotap_namespace(&iter,&power);
      //		else if (iter.current_namespace == &vns_array[0])
//			print_test_namespace(&iter);
	}
	if (err != -ENOENT) {
		printf("Radiotap malformato\n");
		return ;
	}
    time(&power.timestamp);
    std::cout << "Prova " << power.antenna_signal << power.antenna_noise  << power.timestamp<< std::endl;
    printf("\tSignal : %d\n",(signed char) power.antenna_signal);
    printf("Noise : %d\n",(signed char) power.antenna_noise);

    std::cout << "Dopo prova";

    struct ieee80211_radiotap_header *radiotapheader;
    radiotapheader = (ieee80211_radiotap_header*) packet;
    printf("Radiotap header len %u\n", radiotapheader->it_len);

    //Aggiunto controllo CRC
    uint32_t crc = crc32(header->len-4-radiotapheader->it_len,packet+radiotapheader->it_len);
    uint32_t received_crc;
    memcpy(&received_crc,&packet[header->len-4],4);
    if(crc!=received_crc){
      printf("0x%x\n", crc);
      printf("0x%x\n", received_crc);
      printf("ERRORE\n");
      return;
    }

    struct ieee80211mac *frame80211;
    struct framectl_bits *ctl;

    frame80211 = (ieee80211mac *) (packet+radiotapheader->it_len/*SIZERADIOTAP*/);
    ctl = (framectl_bits *)&frame80211->framectl;
    printf("\tProtocol version : %u\n", ctl->protocol_version);
    printf("\tType : %u\n", ctl->type);
    printf("\tSubtype : %u\n", ctl->subtype);
    printf("\tDuration :%u\n", frame80211->duration);
    printf("===802.11===\n");
   // printf("\tReceiver : ");
/*    printf("%02x:%02x:%02x:%02x:%02x:%02x\n", frame80211->receiver[0],frame80211->receiver[1],frame80211->receiver[2],frame80211->receiver[3],
            frame80211->receiver[4],frame80211->receiver[5]);
    printf("\tTransmitter : ");
    printf("%02x:%02x:%02x:%02x:%02x:%02x\n", frame80211->transmitter[0],frame80211->transmitter[1],frame80211->transmitter[2],frame80211->transmitter[3],
            frame80211->transmitter[4],frame80211->transmitter[5]);
    printf("\tDestination : ");
    printf("%02x:%02x:%02x:%02x:%02x:%02x\n", frame80211->destination[0],frame80211->destination[1],frame80211->destination[2],frame80211->destination[3],
            frame80211->destination[4],frame80211->destination[5]);
*/
    //Beacon Frame
    if(ctl->type == 0){
        printf("\nManagement frame\n");
        if(ctl->subtype == 8){
            struct beacon_frame *bframe;
            bframe = (beacon_frame *)(packet+radiotapheader->it_len/*SIZERADIOTAP*//*+SIZEBEACONFRAME*/);
            printf("Beacon\n");
            printf("\tReceiver : ");
            printf("%02x:%02x:%02x:%02x:%02x:%02x\n", bframe->receiver[0],bframe->receiver[1],
                    bframe->receiver[2],bframe->receiver[3],bframe->receiver[4],
                    bframe->receiver[5]);
            printf("\tTransmitter : ");
            printf("%02x:%02x:%02x:%02x:%02x:%02x\n", bframe->transmitter[0],bframe->transmitter[1],
                    bframe->transmitter[2],bframe->transmitter[3],bframe->transmitter[4],
                    bframe->transmitter[5]);
            printf("\tDestination : ");
            printf("%02x:%02x:%02x:%02x:%02x:%02x\n", bframe->destination[0],bframe->destination[1],
                    bframe->destination[2],bframe->destination[3],bframe->destination[4],
                    bframe->destination[5]);
            printf("\tSeq control : %u\n", bframe->seq_control);
            printf("\tTimestamp : %llu\n", bframe->timestamp);
            printf("\tBeacon Interval : %u TU\n", bframe->beacon_interval);
            printf("\tCapability info : %u\n", bframe->capability_info);
            printf("\tTag number : %u\n", bframe->tag);
            printf("\tSSID Length : %u\n", bframe->length);
            bframe->ssid[bframe->length]='\0';
            printf("\tSSID = %s\n", bframe->ssid);
          //  Device* d = new Device(bframe);
          //  if(devices.contains(d->mac_address)){
          //      devices.find(d->mac_address);
          //  }
          //  devices.insert({d->mac_address,d});
     //       devices.push_back(*d);
     //
     //     Cerco se ho gia' un device creato con il mac address
     //     Se positivo controllo che sia stato gia' ricevuto un pacchetto beacon e settato come AP
          //  std::string t(bframe->transmitter, bframe->transmitter+6);
           // std::string transmitter_mac = new string;
            auto transmitter_mac = make_hex_string(std::begin(bframe->transmitter), std::end(bframe->transmitter), false,  true);
            std::cout << "Il risultato della funzione e' : " << transmitter_mac << endl;
            auto search = devices.find(transmitter_mac);
           // auto search = devices.find(bframe->transmitter);
            if(search!=devices.end()){
                std::cout << "Trovato,setto beacon";
                if(!search->second->isAP){
                    search->second->setAP(bframe->length,bframe->ssid);
                    search->second->addPowerValues(power);
                }
            }
            else{
                std::cout << "Non trovato, aggiungo beacon";
                Device* d = new Device(transmitter_mac);
                d->setAP(bframe->length,bframe->ssid);
                d->addPowerValues(power);
            //    std::string s(d->mac_address, d->mac_address+6);
                devices.insert({transmitter_mac,d});
            }
            return;

        }
        if(ctl->subtype == 1){
          printf("Association Response\n");
          struct association_frame *frame;
          frame = (association_frame *)(packet+radiotapheader->it_len);
          if(frame->response == 0){
            printf("Associazione corretta\n");
            auto transmitter_mac = make_hex_string(std::begin(frame->transmitter), std::end(frame->transmitter), false,  true);
            auto search = devices.find(transmitter_mac);
            if(search == devices.end()){
              Device * d = new Device(transmitter_mac);
              d->addPowerValues(power);
              devices.insert({transmitter_mac,d});
            }
            auto receiver_mac = make_hex_string(std::begin(frame->receiver), std::end(frame->receiver), false, true);
            auto search2 = devices.find(receiver_mac);
            if(search2==devices.end()){
              Device* d = new Device(receiver_mac);
              devices.insert({receiver_mac,d});
            }
            search = devices.find(transmitter_mac);
            search2 = devices.find(receiver_mac);
            if(!search->second->isTalking(receiver_mac)){
              search->second->addTalker(receiver_mac);
              search2 = devices.find(receiver_mac);
              search2->second->addTalker(transmitter_mac);
            }
          }
            return;

        }
        if(ctl->subtype == 10){

            printf("Disassociazione\n");
            struct disassociation_frame *frame;
            frame = (disassociation_frame *)(packet+radiotapheader->it_len);
            auto destination_mac = make_hex_string(std::begin(frame->destination), std::end(frame->destination), false, true);
            auto transmitter_mac = make_hex_string(std::begin(frame->transmitter), std::end(frame->transmitter), false,  true);
            auto search = devices.find(transmitter_mac);
            if(search == devices.end()){
              return;
            }
            auto search2 = devices.find(destination_mac);
            if(search2 == devices.end()){
              return;
            }
            search = devices.find(transmitter_mac);
            search2 = devices.find(destination_mac);
            if(search->second->isTalking(destination_mac)){
              printf("Erano connessi, disassocio");
              search->second->removeTalker(destination_mac);
              search2->second->removeTalker(transmitter_mac);
            }
            return;
        }

        if(ctl->subtype == 12){

          printf("Deautenticazione\n");
          //Riutilizzo control frame
          struct control_frames *frame;
          frame = (control_frames *) (packet + radiotapheader->it_len);
          auto receiver_mac= make_hex_string(std::begin(frame->receiver), std::end(frame->receiver), false,  true);
          auto transmitter_mac = make_hex_string(std::begin(frame->transmitter), std::end(frame->transmitter), false,  true);

          //Potrei aggiungerli comunque ai devices se non presenti e non metterli come talkers
          auto search = devices.find(transmitter_mac);
          if(search == devices.end()){
            return;
          }
          auto search2 = devices.find(receiver_mac);
          if(search2 == devices.end()){
            return;
          }
          search = devices.find(transmitter_mac);
          search2 = devices.find(receiver_mac);
          if(search->second->isTalking(receiver_mac)){
            printf("Erano connessi, deautenticazione");
            search->second->removeTalker(receiver_mac);
            search2->second->removeTalker(transmitter_mac);
          }
        return;
        }
    }
    if(ctl->type == 1){
        printf("\nControl frame\n");
        struct control_frames *frame;
        if(ctl->subtype == 8){
             printf("Block ack req");
         //    struct control_frames *frame;
             frame = (control_frames *)(packet+radiotapheader->it_len/*SIZERADIOTAP*/);
             printf("Block ack\n");
             printf("\tReceiver : ");
             printf("%02x:%02x:%02x:%02x:%02x:%02x\n", frame->receiver[0],frame->receiver[1],
                      frame->receiver[2],frame->receiver[3],frame->receiver[4],
                      frame->receiver[5]);
             printf("\tTransmitter : ");
             printf("%02x:%02x:%02x:%02x:%02x:%02x\n", frame->transmitter[0],frame->transmitter[1],
                      frame->transmitter[2],frame->transmitter[3],frame->transmitter[4],
                      frame->transmitter[5]);
        }

        if(ctl->subtype == 0){
          printf("Reserved \n");
          return;
        }
        if(ctl->subtype == 4){
          printf("Beamforming \n");
          return;
        }

        if(ctl->subtype == 7){
          printf("Control wrapper \n");
          return;
        }
        if(ctl->subtype == 9){
        //    struct control_frames *frame;
            frame = (control_frames *)(packet+radiotapheader->it_len/*SIZERADIOTAP*/);
            printf("Block ack\n");
            printf("\tReceiver : ");
            printf("%02x:%02x:%02x:%02x:%02x:%02x\n", frame->receiver[0],frame->receiver[1],
                     frame->receiver[2],frame->receiver[3],frame->receiver[4],
                     frame->receiver[5]);
            printf("\tTransmitter : ");
            printf("%02x:%02x:%02x:%02x:%02x:%02x\n", frame->transmitter[0],frame->transmitter[1],
                     frame->transmitter[2],frame->transmitter[3],frame->transmitter[4],
                     frame->transmitter[5]);
          //  printf("\tBlockAck Control : %u\n", frame->blockack_control);
        }
        if(ctl->subtype == 11){
            printf("RTS\n");
     //       struct control_frames *frame;
            frame = (control_frames *)(packet+radiotapheader->it_len/*SIZERADIOTAP*/);
            printf("\tReceiver : ");
            printf("%02x:%02x:%02x:%02x:%02x:%02x\n", frame->receiver[0],frame->receiver[1],
                    frame->receiver[2],frame->receiver[3],frame->receiver[4],
                    frame->receiver[5]);
            printf("\tTransmitter : ");
            printf("%02x:%02x:%02x:%02x:%02x:%02x\n", frame->transmitter[0],frame->transmitter[1],
                    frame->transmitter[2],frame->transmitter[3],frame->transmitter[4],
                    frame->transmitter[5]);
        }
        if(ctl->subtype == 12){
            printf("Clear to send\n");
            return;
        }
        if(ctl->subtype == 13){
            printf("Ack\n");
            return;
        }
       // auto search = devices.find(frame->transmitter);
//                       std::string t(frame->transmitter, frame->transmitter+6);

        //auto search = devices.find(t);
        auto transmitter_mac = make_hex_string(std::begin(frame->transmitter), std::end(frame->transmitter), false,  true);
        auto search = devices.find(transmitter_mac);
        std::cout << "Cerco";
        if(search == devices.end()){
            //printf("Non trovato, aggiungo\n");
            std::cout << "Non trovato,aggiungo" << std::endl;
            if(isValidMAC(transmitter_mac)){
                Device *d = new Device(transmitter_mac);
                devices.insert({transmitter_mac,d});
            }
            //Device *d = new Device(frame->transmitter);
           // devices.insert({t,d});
         //   Device *d = new Device(/*frame*/ frame->transmitter);
         //   devices.insert({d->mac_address,d});
        }
        auto receiver_mac = make_hex_string(std::begin(frame->receiver), std::end(frame->receiver), false,  true);
        auto search2= devices.find(receiver_mac);
        //std::string r(frame->receiver, frame->receiver+6);
        //auto search2= devices.find(r);
        if(search2 == devices.end()){
            if(isValidMAC(receiver_mac)){
                Device *d = new Device(receiver_mac);
                d->addPowerValues(power);
                devices.insert({receiver_mac,d});
            }
           // Device *d = new Device(frame->receiver);
           // devices.insert({r,d});
        }
//        search = devices.find(frame->receiver);
//        if(search == devices.end()){
//            Device *d = new Device(/*frame*/frame->receiver);
//            devices.insert({d->mac_address,d});
//        }

      //Non e' qui
        if(transmitter_mac.compare(receiver_mac)!=0){
        search = devices.find(transmitter_mac);
        search2 = devices.find(receiver_mac);
        if(search!=devices.end() && search2!=devices.end()){
        if(!search2->second->isTalking(transmitter_mac)){
            search->second->addTalker(receiver_mac);
            search2->second->addTalker(transmitter_mac);
            search2->second->addPowerValues(power);
        }
      }

    }
  }
    if(ctl->type == 2){
        printf("\nData frame\n");
        if(ctl->subtype == 0){
            printf("Data\n");
        }
        if(ctl->subtype == 4){
            printf("Null no data\n");
        }

        if(ctl->subtype == 8){
            printf("QoS data\n");
        }
        if(ctl->subtype == 12){
            printf("QoS null no data\n");
        }
        struct data_frames *frame;
        frame = (data_frames *)(packet+radiotapheader->it_len/*SIZERADIOTAP*/);

            printf("\tReceiver : ");
             printf("%02x:%02x:%02x:%02x:%02x:%02x\n", frame->receiver[0],frame->receiver[1],
                     frame->receiver[2],frame->receiver[3],frame->receiver[4],
                     frame->receiver[5]);
             printf("\tTransmitter : ");
             printf("%02x:%02x:%02x:%02x:%02x:%02x\n", frame->transmitter[0],frame->transmitter[1],
                     frame->transmitter[2],frame->transmitter[3],frame->transmitter[4],
                     frame->transmitter[5]);
             printf("\tDestination : ");
             printf("%02x:%02x:%02x:%02x:%02x:%02x\n", frame->destination[0],frame->destination[1],
                     frame->destination[2],frame->destination[3],frame->destination[4],
                     frame->destination[5]);
  //              std::string t(frame->transmitter, frame->transmitter+6);
  //              auto search = devices.find(t);
            auto transmitter_mac = make_hex_string(std::begin(frame->transmitter), std::end(frame->transmitter), false,  true);
            auto search = devices.find(transmitter_mac);
         std::cout << "Cerco";
         if(search == devices.end()){
             //printf("Non trovato, aggiungo\n");
             std::cout << "Non trovato,aggiungo" << std::endl;
             if(isValidMAC(transmitter_mac)){
                Device *d = new Device(transmitter_mac);
                devices.insert({transmitter_mac,d});
             }
           //  Device *d = new Device(frame->transmitter);
           //  devices.insert({t,d});
          //   Device *d = new Device(/*frame*/ frame->transmitter);
          //   devices.insert({d->mac_address,d});
         }
        auto receiver_mac = make_hex_string(std::begin(frame->receiver), std::end(frame->receiver), false,  true);
        auto search2 = devices.find(receiver_mac);
         //std::string r(frame->receiver, frame->receiver+6);
         //auto search2= devices.find(r);
         if(search2 == devices.end()){
             if(isValidMAC(receiver_mac)){
          Device *d = new Device(receiver_mac);
          d->addPowerValues(power);

          devices.insert({receiver_mac,d});
             }
             //   Device *d = new Device(frame->receiver);
          //   devices.insert({r,d});
         }

        auto destination_mac = make_hex_string(std::begin(frame->destination), std::end(frame->destination), false, true);
        auto search3 = devices.find(destination_mac);
        if(search3 == devices.end()){
            if(isValidMAC(destination_mac)){
                Device *d =new Device(destination_mac);
                devices.insert({destination_mac,d});
            }
        }

         search = devices.find(transmitter_mac);
         search2 = devices.find(receiver_mac);
         search3 = devices.find(destination_mac);
        if(search!=devices.end() && search2!=devices.end() && search3!=devices.end()){
        if((!search2->second->isTalking(transmitter_mac))&&(transmitter_mac.compare(receiver_mac)!=0)){
             search->second->addTalker(receiver_mac);
             search2->second->addTalker(transmitter_mac);
             search2->second->addPowerValues(power);
        }
        if((!search3->second->isTalking(transmitter_mac))&&(transmitter_mac.compare(destination_mac)!=0)){
            search->second->addTalker(destination_mac);
            search3->second->addTalker(transmitter_mac);
        }
    }
    }
}

RadiotapScanner::RadiotapScanner(){
  //Lookup device
  device = pcap_lookupdev(errbuf);
  if(device == NULL){
      cout << ("Device di rete non trovato\n");
    //  return 1;
  }
  cout << ("Device scelto : ") << device << endl;

  //Get Netmask
  if(pcap_lookupnet(device, &network, &mask, errbuf) == -1) {
   cout << ("Errore netmask del device") << device << endl;
   network = 0;
   mask = 0;
  }

  //Create and set up device
  handle = pcap_create(device, errbuf);
  //Monitor mode
  pcap_set_rfmon(handle, 1);
  //Promiscous mode
  pcap_set_promisc(handle, 1);
  //Snaplen
  pcap_set_snaplen(handle, 2048);
  //Timeout
  pcap_set_timeout(handle, 1000);

  pcap_activate(handle);

  //Compile filter
  if(pcap_compile(handle,&fp,filter,0,network) == PCAP_ERROR){
      cout << ("Errore compile filtro") << filter << endl;
      cout << pcap_geterr(handle);
    //  return 1;
  }

  //Set filter
  if(pcap_setfilter(handle,&fp) == -1){
      cout << ("Errore set filtro") << endl;
    //  return 1;
  }

}

RadiotapScanner::RadiotapScanner(char *arg){
  string file;
  file=arg;
  pcap_t * pcap = pcap_open_offline(file.c_str(), errbuf);
  struct pcap_pkthdr *header;
  const u_char *data;

  //Forza arp del repeater per wifi_repeater.pcap
  arp.push_back("70:4f:57:2e:2d:66");
  //

  u_int packetCount = 0;
  int returnValue;
      while ((returnValue = pcap_next_ex(pcap, &header, &data) >= 0) /*&& (packetCount<10)*/)
  {
      // Show the packet number
      printf("Packet # %i\n", ++packetCount);
      dissectpacket(NULL,header,data);
  }
  std::cout << "Stampa di packResults" << std::endl;
  packResults();
/*
  printf("Stampa finale\n");
  printf("Size :%lu\n", devices.size());
      for( const auto& n : devices ) {
      std::cout << "=====================================" << std::endl;
      std::cout << "Key:[" << n.first << "]" << std::endl;
      n.second->Print();
      std::cout << "=====================================" << std::endl;
  }*/
}

void RadiotapScanner::packResults(){
  std::vector<Device *> ap;
  for( const auto& n : devices ) {
    if(n.second->isAP){
      ap.push_back(n.second);
    }
  }
  for(const auto n : ap){
    std::cout << n->getDeviceMAC() << " " << n->getDeviceSSID() << std::endl;
    std::string mac = n->getDeviceMAC();
    checkLocalAdministered(mac);
/*
    for(auto v : n->talkers){
      //std::cout << v.getDeviceMAC() << " " << v.getDeviceSSID() << std::endl;
      std::string mac = v;
      checkLocalAdministered(mac);
  }*/
}
  for(const auto i : devices){
    if(i.second->isLocallyAdministered){
          findGloballyAdministeredInterface(i.second->mac_address);
        }
  }

  for( const auto n : ap ){
    findMainMACAP(n->getDeviceMAC());
  }
/*
  for( const auto n : devices ){
    findMainMACAP(n.second->getDeviceMAC());
  }*/

}

void RadiotapScanner::startScan(){
  pcap_loop(handle,NPACKETS,dissectpacket,NULL);
}
void RadiotapScanner::close(){
  pcap_set_rfmon(handle,0);
  pcap_set_promisc(handle,0);
  pcap_freecode(&fp);
  pcap_close(handle);
}

void RadiotapScanner::feedARPResults(vector<std::string> arp_r){
  arp=arp_r;
}

std::unordered_map<std::string,Device*> RadiotapScanner::getResult(){
  return devices;
}