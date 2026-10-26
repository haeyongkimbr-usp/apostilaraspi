#include <cekeikon.h>

int main()
{ int n=401;
  Mat_<COR> a(n,n,COR(255,255,255));
  ImgLcb<COR> ae(a); ae.centro();
  for (int x=ae.minx; x<=ae.maxx; x++)
    for (int y=ae.miny; y<=ae.maxy; y++) {
      int m=max(abs(x),abs(y));
      if (m>170) ae(x,y)=COR(255,255,254);
      else if (m>110) ae(x,y)=COR(0,0,1);
      else if (m>80) ae(x,y)=COR(255,254,255);
      else if (m==80) ae(x,y)=COR(128,255,255);
    }
  imp(a,"quadrado.png");
}
