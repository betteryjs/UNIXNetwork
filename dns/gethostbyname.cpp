//
// Created by yjs on 2022/1/30.
//

#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
using namespace std;

int main(int argc, char **agrv) {

  //  char *ptr,**pptr;
  char **pptr;
  string host_name;
  char str[INET_ADDRSTRLEN];
  hostent *hptr;
  for (int i = 1; i < argc; ++i) {
    host_name = string(agrv[i]);
    if ((hptr = gethostbyname(host_name.c_str())) == nullptr) {
      cerr << "gethostbyname : " << host_name << hstrerror(h_errno) << endl;
      continue;
    }
    cout << "official hostname : " << hptr->h_name << endl;
    for (pptr = hptr->h_aliases; *pptr != nullptr; pptr++) {
      cout << "\talias : " << *pptr << endl;
    }

    switch (hptr->h_addrtype) {
    case AF_INET:
      pptr = hptr->h_addr_list;
      for (; *pptr != nullptr; pptr++) {
        cout << "\taddress : "
             << inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)) << endl;
      }

      break;

    default:
      perror("unknown address type");
      break;
    }
  }

  return 0;
}