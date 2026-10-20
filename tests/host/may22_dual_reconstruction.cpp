// May 22 2026 dual-edge reconstruction.
// Donor e78b6f686ee99fa4463d6fd4773d813fa0e8b319.
// Float stand-in for SQ15x16. Not a byte-identical S2 image.
// Primary Bloom, secondary Waveform-Fast. Orientation of primary as top is unproven.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace {
constexpr int N = 160;
constexpr int FPS = 40;
constexpr float INC_R = 1.f, INC_G = 0.4453f, INC_B = 0.1562f;
struct RGB { float r=0, g=0, b=0; };
struct Edge {
  RGB show[N]{};
  RGB history[N]{};
  RGB last{};
  float peak_last = 0;
};
struct Settings {
  float photons=0.2f, chroma=0, mood=0.12f, saturation=1, square=1;
  bool mirror=true, palette=false, incandescent_mode=false;
  float incandescent=0;
  bool secondary_clean=false;
};

float wrap01(float h) { h-=std::floor(h); return h; }
RGB hsv(float h, float s, float v) {
  h = wrap01(h); s=std::clamp(s,0.f,1.f); v=std::max(0.f,v);
  float H=h*6.f; int i=int(H); float f=H-i;
  float p=v*(1-s), q=v*(1-f*s), t=v*(1-(1-f)*s);
  float r,g,b;
  switch(i%6){
    case 0: r=v;g=t;b=p; break; case 1: r=q;g=v;b=p; break; case 2: r=p;g=v;b=t; break;
    case 3: r=p;g=q;b=v; break; case 4: r=t;g=p;b=v; break; default: r=v;g=p;b=q; break;
  }
  return {r,g,b};
}
RGB force_sat(RGB c, float s) {
  float m=std::max(c.r,std::max(c.g,c.b));
  if(m<=1e-6f) return {};
  float h,sat,v=m; float mn=std::min(c.r,std::min(c.g,c.b)); float d=m-mn;
  if(d<=1e-6f) { h=0; sat=0; }
  else {
    if(m==c.r) h=(c.g-c.b)/d; else if(m==c.g) h=2+(c.b-c.r)/d; else h=4+(c.r-c.g)/d;
    h/=6.f; if(h<0) h+=1; sat=d/m;
  }
  (void)sat; return hsv(h,s,v);
}
void add_sprite(RGB* dest, const RGB* sprite, float position, float alpha) {
  int whole=int(position); float fract=position-whole;
  float mix_r=fract, mix_l=1.f-fract;
  for(int i=0;i<N;i++){
    int L=i+whole, R=L+1;
    if(L>=0 && L<N){ dest[L].r+=sprite[i].r*mix_l*alpha; dest[L].g+=sprite[i].g*mix_l*alpha; dest[L].b+=sprite[i].b*mix_l*alpha; }
    if(R>=0 && R<N){ dest[R].r+=sprite[i].r*mix_r*alpha; dest[R].g+=sprite[i].g*mix_r*alpha; dest[R].b+=sprite[i].b*mix_r*alpha; }
  }
}
void mirror_down(RGB* a) {
  RGB tmp[N];
  for(int i=0;i<N/2;i++){ tmp[N/2+i]=a[N/2+i]; tmp[N/2-1-i]=a[N/2+i]; }
  std::memcpy(a,tmp,sizeof(tmp));
}
RGB palette10(float x) {
  // es_vintage_01_gp stops, index 10 at e78b6f6.
  struct S { int i; int r,g,b; } s[]={{0,4,1,1},{51,16,0,1},{76,97,104,3},{101,255,131,19},{127,67,9,4},{153,16,0,1},{229,4,1,1},{255,4,1,1}};
  float p=std::clamp(x,0.f,1.f)*255.f;
  int k=0; while(k<7 && s[k+1].i<p) k++;
  float t=s[k+1].i==s[k].i?0:(p-s[k].i)/(s[k+1].i-s[k].i);
  auto L=[&](int a,int b){return (a+(b-a)*t)/255.f;};
  return {L(s[k].r,s[k+1].r), L(s[k].g,s[k+1].g), L(s[k].b,s[k+1].b)};
}
void soft_clip(RGB& c) {
  c.r=std::max(0.f,c.r); c.g=std::max(0.f,c.g); c.b=std::max(0.f,c.b);
  float m=std::max(c.r,std::max(c.g,c.b));
  if(m>0.85f){ float cm=std::min(1.f, 0.85f+(m-0.85f)*0.4f); float s=cm/m; c.r*=s; c.g*=s; c.b*=s; }
}
void bloom(Edge& e, const float chroma[12], const Settings& s, bool keep_history) {
  RGB out[N]{};
  float shift=(0.250f+1.750f*s.mood)*(float(N)/128.f);
  add_sprite(out, e.history, shift, 0.99f);
  RGB sum{};
  for(int i=0;i<12;i++){
    float bin=std::max(0.f, chroma[i]);
    RGB add = s.palette ? RGB{} : hsv(i/12.f, s.saturation, bin*bin*(1.f/6.f));
    sum.r+=add.r; sum.g+=add.g; sum.b+=add.b;
  }
  sum.r=std::min(1.f,sum.r); sum.g=std::min(1.f,sum.g); sum.b=std::min(1.f,sum.b);
  if(!s.palette) for(int i=0;i<int(s.square);i++){ sum.r*=sum.r; sum.g*=sum.g; sum.b*=sum.b; }
  else sum=palette10(0);
  RGB insert=sum;
  if(!s.palette) insert=force_sat(sum, s.saturation);
  out[N/2-1]=insert; out[N/2]=insert;
  if(keep_history) std::memcpy(e.history, out, sizeof(out));
  int fade=N/4;
  for(int i=0;i<fade;i++){
    float prog=float(i)/(fade-1); float a=prog*prog;
    out[N-1-i].r*=a; out[N-1-i].g*=a; out[N-1-i].b*=a;
    out[i].r*=a; out[i].g*=a; out[i].b*=a;
  }
  mirror_down(out);
  std::memcpy(e.show, out, sizeof(out));
}
void shift_upper(RGB* a, int steps) {
  int half=N/2;
  for(int s=0;s<steps;s++){
    for(int i=N-1;i>half;i--) a[i]=a[i-1];
    a[half]={};
  }
}
void waveform(Edge& e, const float chroma[12], float peak, float raw, const Settings& s) {
  float smoothed=peak*0.05f + e.peak_last*0.95f; e.peak_last=smoothed;
  float abs_amp=std::min(1.f, std::fabs(peak));
  bool reactive = raw > 750.f*1.10f && abs_amp>=0.08f;
  RGB current{}; float bright_sum=0;
  for(int c=0;c<12;c++){
    float bright=std::max(0.f, chroma[c]);
    for(int k=0;k<int(std::floor(s.square));k++) bright*=bright;
    float fract=s.square-std::floor(s.square);
    if(fract>0.01f) bright=bright*(1-fract)+(bright*bright)*fract;
    bright=std::min(1.f, bright*1.5f);
    if(s.palette) bright_sum+=bright/12.f;
    else { RGB n=hsv(c/12.f, s.saturation, bright); current.r+=n.r/12.f; current.g+=n.g/12.f; current.b+=n.b/12.f; }
  }
  if(s.palette) current=palette10(bright_sum);
  float energy=0; for(int c=0;c<12;c++) energy+=std::max(0.f,chroma[c]); energy/=12.f;
  float blend=std::min(1.f, energy*2.f);
  float fb_v=reactive?abs_amp:0.f;
  RGB fallback = s.palette ? palette10(s.chroma) : hsv(s.chroma, s.saturation, fb_v);
  if(s.palette){ fallback.r*=fb_v; fallback.g*=fb_v; fallback.b*=fb_v; }
  e.last.r=current.r*blend+fallback.r*(1-blend);
  e.last.g=current.g*blend+fallback.g*(1-blend);
  e.last.b=current.b*blend+fallback.b*(1-blend);
  float fade = reactive ? (1.f-0.10f*abs_amp) : 0.85f;
  for(int i=0;i<N;i++){ e.show[i].r*=fade; e.show[i].g*=fade; e.show[i].b*=fade; }
  if(s.mirror) shift_upper(e.show, 1);
  if(reactive){
    float amp=std::clamp(peak,-1.f,1.f);
    int pos=int(std::lround(N/2.f + amp*(N/2.f)));
    pos=std::clamp(pos,0,N-1);
    if(s.mirror) pos=std::max(pos, N/2);
    e.show[pos]=e.last;
  }
  if(s.mirror) mirror_down(e.show);
}
void finish(RGB* a, const Settings& s, bool secondary) {
  float photons=s.photons*s.photons;
  float inc = secondary && s.secondary_clean ? 0.f : s.incandescent;
  for(int i=0;i<N;i++){
    a[i].r*=photons; a[i].g*=photons; a[i].b*=photons;
    if(!s.incandescent_mode && inc>0){
      a[i].r=a[i].r*(1-inc)+a[i].r*INC_R*inc;
      a[i].g=a[i].g*(1-inc)+a[i].g*INC_G*inc;
      a[i].b=a[i].b*(1-inc)+a[i].b*INC_B*inc;
    }
    soft_clip(a[i]);
  }
}
void chroma_from(const int16_t* pcm, int n, float rate, float out[12]) {
  std::fill(out,out+12,0.f);
  if(n<32) return;
  const float notes[12]={261.63f,277.18f,293.66f,311.13f,329.63f,349.23f,369.99f,392.f,415.3f,440.f,466.16f,493.88f};
  for(int pc=0; pc<12; pc++) for(int oct=0; oct<3; oct++){
    float f=notes[pc]*std::pow(2.f, oct-1);
    if(f>=rate*0.45f) continue;
    float w=2.f*float(M_PI)*f/rate, c=0,s=0;
    for(int i=0;i<n;i++){ float x=pcm[i]/32768.f; c+=x*std::cos(w*i); s+=x*std::sin(w*i); }
    out[pc]+=std::sqrt(c*c+s*s)/n;
  }
  float m=*std::max_element(out,out+12);
  if(m>1e-8f) for(int i=0;i<12;i++) out[i]=std::min(1.f, out[i]/m);
}
uint16_t code(float x){ x=std::clamp(x,0.f,1.f); return uint16_t(std::lround(x*65535.f)); }
void write_px(FILE* f, const RGB* a){ for(int i=0;i<N;i++){ uint16_t c[3]={code(a[i].r),code(a[i].g),code(a[i].b)}; std::fwrite(c,2,3,f);} }

void prove_history() {
  Edge live, dead; float ch[12]{}; ch[0]=1;
  Settings s; s.photons=1; s.square=0; s.mood=0.5f; s.incandescent=0;
  for(int i=0;i<40;i++){ bloom(live,ch,s,true); bloom(dead,ch,s,false); }
  float away=0, dead_away=0;
  for(int i=N/2+8;i<N/2+24;i++) away+=live.show[i].r+live.show[i].g+live.show[i].b;
  for(int i=N/2+8;i<N/2+24;i++) dead_away+=dead.show[i].r;
  if(!(away>1.f && dead_away<0.05f)) { std::fprintf(stderr,"history mutation failed away=%g dead=%g\n", away, dead_away); std::exit(1); }
}
}

int main(int argc, char** argv) {
  if(argc<3){ std::fprintf(stderr,"usage: may22 pcm output-prefix\n"); return 2; }
  prove_history();
  std::ifstream in(argv[1], std::ios::binary);
  std::vector<char> bytes((std::istreambuf_iterator<char>(in)), {});
  if(bytes.size()<4 || bytes.size()%2){ std::fprintf(stderr,"pcm\n"); return 2; }
  std::vector<int16_t> pcm(bytes.size()/2);
  std::memcpy(pcm.data(), bytes.data(), bytes.size());
  const int rate=24000;
  const int frames=int(pcm.size()/rate)*FPS;
  Settings primary{0.200195f,0.f,0.116638f,1.f,1.f,true,false,false,0.50f,false};
  Settings secondary{1.f,0.033325f,0.216614f,1.f,1.f,true,true,false,0.f,true};
  Edge top, bottom;
  float chroma[12]{}, smooth[12]{};
  FILE* raw=std::fopen((std::string(argv[2])+".rgb16le").c_str(),"wb");
  if(!raw) return 2;
  int hop=180; int sample=0; float peak=0, raw_peak=0;
  for(int frame=0; frame<frames; frame++){
    int end=std::min<int>((frame+1)*rate/FPS, pcm.size());
    while(sample+hop<=end){
      chroma_from(pcm.data()+sample, hop, rate, chroma);
      for(int i=0;i<12;i++) smooth[i]=smooth[i]*0.8f+chroma[i]*0.2f;
      float mx=0; for(int i=0;i<hop;i++) mx=std::max(mx, std::fabs(pcm[sample+i]/32768.f));
      peak=mx; raw_peak=mx*32768.f; sample+=hop;
    }
    bloom(top, smooth, primary, true);
    waveform(bottom, smooth, peak, raw_peak, secondary);
    RGB shown_top[N], shown_bottom[N];
    std::memcpy(shown_top, top.show, sizeof(shown_top));
    std::memcpy(shown_bottom, bottom.show, sizeof(shown_bottom));
    finish(shown_top, primary, false);
    finish(shown_bottom, secondary, true);
    write_px(raw, shown_top); write_px(raw, shown_bottom);
  }
  std::fclose(raw);
  std::printf("frames %d fps %d primary-photons-sq %.6f\n", frames, FPS, primary.photons*primary.photons);
  return 0;
}
