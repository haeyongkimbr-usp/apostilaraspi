//projeto.hpp - incluir no seu projeto
#include "raspberry.hpp"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <signal.h>
#include <vector>
//#include <opencv2/opencv.hpp>
//using namespace cv;

class DEVICE {
public:
  // get sockaddr, IPv4 or IPv6:
  static void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
      return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
  }

  virtual void sendBytes(int nBytesToSend, BYTE *buf)=0;
  virtual void receiveBytes(int nBytesToReceive, BYTE *buf)=0;

  //void sendChar(char ch);
  void sendUint(uint32_t m);
  //void sendInt(int32_t m);
  void sendVb(const vector<BYTE>& vb);
  //void sendString(const string& st);
  //void sendImg(const Mat_<Vec3b>& img);
  //void sendImgComp(const Mat_<Vec3b>& img);
  //void receiveChar(char& ch);
  void receiveUint(uint32_t& m);
  //void receiveInt(int32_t& m);
  void receiveVb(vector<BYTE>& vb);
  //void receiveString(string& st);
  //void receiveImg(Mat_<Vec3b>& img);
  //void receiveImgComp(Mat_<Vec3b> &img);
};

/*
void DEVICE::sendChar(char ch) {
  static char n;
  n=ch;
  sendBytes(1,(BYTE*)&ch);
}
*/

void DEVICE::sendUint(uint32_t m) {
  static uint32_t n;
  n=htonl(m);
  sendBytes(4,(BYTE*)&n);
}

/*
void DEVICE::sendInt(int32_t m) {
  static int32_t n;
  n=htonl(m);
  sendBytes(4,(BYTE*)&n);
}
*/

void DEVICE::sendVb(const vector<BYTE>& vb) {
  sendUint(vb.size());
  sendBytes(vb.size(),(BYTE*)vb.data());
}

/*
void DEVICE::sendString(const string& st) {
  sendUint(st.length());
  sendBytes(st.length(),(BYTE*)st.data());
}

void DEVICE::sendImg(const Mat_<Vec3b>& img) {
  if (!img.isContinuous()) erro("Erro sendImg: Imagem nao-continua");
  sendUint(img.rows);
  sendUint(img.cols);
  sendBytes(3*img.total(),img.data);
}

void DEVICE::sendImgComp(const Mat_<Vec3b>& img) {
  if (!img.isContinuous()) erro("Erro sendImgComp: Imagem nao-continua");
  vector<BYTE> vb;
  vector<int> param{CV_IMWRITE_JPEG_QUALITY,80};
  imencode(".jpg",img,vb,param);
  sendVb(vb);
}

void DEVICE::receiveChar(char& ch) {
  receiveBytes(1,(BYTE*)&ch);
}
*/

void DEVICE::receiveUint(uint32_t& m) {
  receiveBytes(4,(BYTE*)&m);
  m=ntohl(m);
}

/*
void DEVICE::receiveInt(int32_t& m) {
  receiveBytes(4,(BYTE*)&m);
  m=ntohl(m);
}
*/

void DEVICE::receiveVb(vector<BYTE>& vb) {
  uint32_t t;
  receiveUint(t);
  vb.resize(t);
  receiveBytes(vb.size(),(BYTE*)vb.data());
}

/*
void DEVICE::receiveString(string& st) {
  uint32_t t;
  receiveUint(t);
  st.resize(t);
  receiveBytes(st.size(),(BYTE*)st.data());
}

void DEVICE::receiveImg(Mat_<Vec3b>& img) {
  uint32_t nl; receiveUint(nl);
  uint32_t nc; receiveUint(nc);
  img.create(nl,nc);
  receiveBytes(3*img.total(),img.data);
}

void DEVICE::receiveImgComp(Mat_<Vec3b> &img) {
  vector<BYTE> vb;
  receiveVb(vb);
  img=imdecode(vb,1);
}
*/

class SERVER: public DEVICE {
  const string PORT="3490";  // the port users will be connecting to
  const int BACKLOG=1;   // how many pending connections queue will hold

  int listensock, sockfd;  // listen on sock_fd, new connection on sockfd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;

public:
  SERVER() {
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    if ((rv = getaddrinfo(NULL, PORT.c_str(), &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); exit(1);
    }
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((listensock = socket(p->ai_family, p->ai_socktype,
          p->ai_protocol)) == -1) {
        perror("server: socket"); continue;
      }
      if (setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &yes,
          sizeof(int)) == -1) {
        perror("setsockopt"); exit(1);
      }
      if (bind(listensock, p->ai_addr, p->ai_addrlen) == -1) {
        close(listensock); perror("server: bind"); continue;
      }
      break;
    }
    freeaddrinfo(servinfo); // all done with this structure
    if (p == NULL)  {
      fprintf(stderr, "server: failed to bind\n"); exit(1);
    }
    if (listen(listensock, BACKLOG) == -1) {
      perror("listen"); exit(1);
    }
  }

  ~SERVER() {
    close(sockfd);
    printf("server: fechei conexao.\n");
  }

  void waitConnection() {
    printf("server: Esperando conexao...\n");
    while (true) {
      sin_size = sizeof their_addr;
      sockfd = accept(listensock, (struct sockaddr *)&their_addr, &sin_size);
      if (sockfd == -1) {
        perror("accept"); continue;
      } else break;
    }
    inet_ntop(their_addr.ss_family,
      get_in_addr((struct sockaddr *)&their_addr),
      s, sizeof s);
    printf("server: recebi conexao de %s\n", s);
    close(listensock);
  }

  void sendBytes(int nBytesToSend, BYTE *buf) {
    while (nBytesToSend>0) {
      int nSentBytes = send(sockfd, buf, nBytesToSend, 0);
      //cout << "SERVER nSentBytes=" << nSentBytes << endl;
      if (nSentBytes==-1) {
        perror("camserver: send"); exit(1);
      }
      nBytesToSend -= nSentBytes;
      buf += nSentBytes;
    }
  }

  void receiveBytes(int nBytesToReceive, BYTE *buf) {
    while (nBytesToReceive>0) {
      int nReceivedBytes = recv(sockfd, buf, nBytesToReceive, 0);
      //cout << "SERVER nReceivedBytes=" << nReceivedBytes << endl;
      if (nReceivedBytes==-1) { perror("recv"); exit(1); }
      nBytesToReceive -= nReceivedBytes;
      buf += nReceivedBytes;
    }
  }

};

class CLIENT: public DEVICE {
  const string PORT="3490"; // the port client will be connecting to

  int sockfd, numbytes;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];

public:
  CLIENT(string endereco) {
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo(endereco.c_str(), PORT.c_str(), &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); exit(1);
    }
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,
          p->ai_protocol)) == -1) {
        perror("client: socket"); continue;
      }
      if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
        perror("client: connect"); close(sockfd); continue;
      }
      break;
    }
    if (p == NULL) {
      fprintf(stderr, "client: failed to connect\n"); exit(1);
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
        s, sizeof s);
    printf("client: conectando a %s\n", s);
    freeaddrinfo(servinfo); // all done with this structure
  }

  ~CLIENT() {
    close(sockfd);
  }

  void sendBytes(int nBytesToSend, BYTE *buf) {
    while (nBytesToSend>0) {
      int nSentBytes = send(sockfd, buf, nBytesToSend, 0);
      //cout << "CLIENT nSentBytes=" << nSentBytes << endl;
      if (nSentBytes==-1) {
        perror("camclient: send"); exit(1);
      }
      nBytesToSend -= nSentBytes;
      buf += nSentBytes;
    }
  }

  void receiveBytes(int nBytesToReceive, BYTE *buf) {
    while (nBytesToReceive>0) {
      int nReceivedBytes = recv(sockfd, buf, nBytesToReceive, 0);
      //cout << "CLIENT nReceivedBytes=" << nReceivedBytes << endl;
      if (nReceivedBytes==-1) { perror("recv"); exit(1); }
      nBytesToReceive -= nReceivedBytes;
      buf += nReceivedBytes;
    }
  }
};

bool testaBytes(BYTE* buf, BYTE b, int n) {
  //Testa se n bytes da memoria buf possuem valor b
  bool igual=true;
  for (unsigned i=0; i<n; i++)
    if (buf[i]!=b) { igual=false; break; }
  return igual;
}

bool testaVb(const vector<BYTE> vb, BYTE b) {
  //Testa se todos os bytes de vb possuem valor b
  bool igual=true;
  for (unsigned i=0; i<vb.size(); i++)
    if (vb[i]!=b) { igual=false; break; }
  return igual;
}

/*
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
class ScaleMatchFLT2 {
  // Scale-invariant matching for Mat_<float>.
 public:
  vector< Mat_<float> > q; // scale-invariant query image q
  double escInic, escFim, passoEsc; int nEsc; float dontCare;
  vector< Mat_<float> > cc; vector< Mat_<float> > ncc;
  Mat_<float> coef; Mat_<short> ind;
  vector<Point> vpto; vector<float> vcc; vector<short> vmaxS; vector<float> vncc;
  const int vmax=20;

  // Gera multi-escala da imagem _q e guarda em q
  // escFim<escInic
  // O resultado vai ter nEsc slices.
  // O tamanho da imagem q[i] em cada slice e' diferente.
  ScaleMatchFLT2(Mat_<float> _q, double _escInic, double _escFim, int _nEsc, float _dontCare=-1);
  bool localizaPlaca(Mat_<Vec3b> a, double limiar2, bool geraSaida, Mat_<Vec3b>& d, Point& maxLoc, int& maxS, double& scale);
};

ScaleMatchFLT2::ScaleMatchFLT2(Mat_<float> _q, double _escInic, double _escFim, int _nEsc, float _dontCare) {
// Gera multiescala de _q e guarda em q
// O resultado vai ter nEsc slices.
// O tamanho da imagem em cada slice e' diferente.
  escInic=_escInic; escFim=_escFim; nEsc=_nEsc; dontCare=_dontCare;
  if (nEsc<=1) erro("Erro: nEsc<=1");
  q.resize(nEsc); // As imagens ainda nao foram alocadas
  passoEsc=exp(log(escFim/escInic)/(nEsc-1));

  #pragma omp parallel for
  for (unsigned s=0; s<q.size(); s++) {
    double mult=escInic*pow(passoEsc,s);
    Mat_<float> t;
    resize(_q, t, Size(cvRound(mult*_q.cols),cvRound(mult*_q.rows)), 0, 0, INTER_NEAREST);
    q[s]=somaAbsDois(dcReject(t));
  }
}

//Gera saida d se d nao for nulo
bool ScaleMatchFLT2::localizaPlaca(Mat_<Vec3b> a, double limiar2, bool geraSaida, Mat_<Vec3b>& d,
  Point& maxLoc, int& maxS, double& maxScale) {
  if (geraSaida) a.copyTo(d);

  Mat_<float> f; converte(a,f);
  cc.resize(nEsc);
  #pragma omp parallel for
  for (int s=0; s<nEsc; s++) {
    cc[s]=matchTemplateSame(f,q[s],CV_TM_CCORR);
  }
  coef.create(f.size()); coef.setTo(-infinito);
  ind.create(f.size());  ind.setTo(0);
  #pragma omp parallel for
  for (int l=0; l<f.rows; l++)
    for (int c=0; c<f.cols; c++)
      for (int s=0; s<nEsc; s++)
        if (coef(l,c)<cc[s](l,c)) {
          coef(l,c)=cc[s](l,c); ind(l,c)=s;
        }
  ncc.resize(nEsc);
  #pragma omp parallel for
  for (int s=0; s<nEsc; s++)
    ncc[s]=matchTemplateSame(f,q[s],CV_TM_CCOEFF_NORMED);

  vpto=kmax(coef, vmax, 10); // vmax=10 pontos separados por pelo menos 10 pixels
  vcc.resize(vmax); vmaxS.resize(vmax); vncc.resize(vmax);
  for (int i=0; i<vmax; i++) {
    Point pt=vpto[i]; int S=ind(pt); vcc[i]=coef(pt); vmaxS[i]=S; vncc[i]=ncc[S](pt);
  }

  bool detectou=false; float maxVal1, maxVal2;
  for (int i=0; i<vmax; i++) {
    Point pt=vpto[i]; int S=ind(pt); float val1=vcc[i]; float val2=vncc[i]; double scale=escInic*pow(passoEsc,S);
    if (val2>=limiar2 && !detectou) { detectou=true; maxLoc=pt;
      maxS=S; maxVal1=val1, maxVal2=val2, maxScale=scale;
    }
    if (geraSaida) {
      int tamanho=arredonda(q[S].rows*0.40);
      Point p(tamanho,tamanho); Vec3b cor{192,192,0}; d(pt.y,pt.x)=cor;
      Rect rect(pt-p, pt+p); int largura=1;
      rectangle(d, rect, toScalar(cor), largura);
      putText(d,format("%d",S),                    pt+Point(1+tamanho,-7),0,0.3,toScalar(cor),1);
      putText(d,format("%02d",arredonda(100*val1)),pt+Point(1+tamanho, 2),0,0.3,toScalar(cor),1);
      putText(d,format("%02d",arredonda(100*val2)),pt+Point(1+tamanho,11),0,0.3,toScalar(cor),1);
    }
  }
  if (detectou && geraSaida) {
    int tamanho=arredonda(q[maxS].rows*0.40);
    Point p(tamanho,tamanho); Vec3b cor=Vec3b{192,255,255}; d(maxLoc.y,maxLoc.x)=cor;
    Rect rect(maxLoc-p, maxLoc+p); int largura=3;
    rectangle(d, rect, toScalar(cor), largura);
    putText(d,format("%d",maxS),                    maxLoc+Point(2+tamanho,-7),0,0.3,toScalar(cor),1);
    putText(d,format("%02d",arredonda(100*maxVal1)),maxLoc+Point(2+tamanho, 2),0,0.3,toScalar(cor),1);
    putText(d,format("%02d",arredonda(100*maxVal2)),maxLoc+Point(2+tamanho,11),0,0.3,toScalar(cor),1);
  }
  return detectou;
}
*/
