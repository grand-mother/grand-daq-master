// Traces.c
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "Traces.h"
#define INTSIZE 4
#define SHORTSIZE 2
#include "fftw3.h"
//#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TClass.h"
#include "TSystem.h"
#include "TRandom.h"
#include "TH1.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "grand_util.h"

int main(int argc, char **argv)
{
  int i;
  int fftlen;
  float *ttrace,*fmag,*fphase;
  double psumt,psumf;
  
  TRandom *noise = new TRandom();
  TFile g("Quest.root","RECREATE");
  fftlen = 1024;
  fft_init(fftlen);
  ttrace = (float *)malloc(fftlen*sizeof(float));
  fmag = (float *)malloc(fftlen*sizeof(float));
  fphase = (float *)malloc(fftlen*sizeof(float));
  for(i=0;i<fftlen;i++) ttrace[i] = 3*sin(i/8.);
  mag_and_phase(ttrace,fmag,fphase);
  TH1F *HT = new TH1F("HT","Trace",fftlen,0.,fftlen);
  TH1F *HF = new TH1F("HF","FFT",fftlen/2,0.,512);
  psumt = 0;
  psumf = 0;
  for(i=0;i<fftlen;i++){
    HT->Fill(i+0.5,ttrace[i]);
    psumt+=ttrace[i]*ttrace[i];
    if(i<fftlen/2){
      psumf+=4*fmag[i]*fmag[i]/(fftlen*fftlen);
      HF->Fill(i+0.5,2*fmag[i]/fftlen);
    }
  }
  printf("1024 Sine: Time %g Freq %g\n",psumt/fftlen,psumf/2);
  HT->Write();
  HF->Write();
  for(i=0;i<fftlen;i++) ttrace[i] = 1.5*noise->Gaus();
  mag_and_phase(ttrace,fmag,fphase);
  TH1F *HNT = new TH1F("HNT","Trace",fftlen,0.,fftlen);
  TH1F *HNF = new TH1F("HNF","FFT",fftlen/2,0.,512);
  psumt = 0;
  psumf = 0;
  for(i=0;i<fftlen;i++){
    HNT->Fill(i+0.5,ttrace[i]);
    psumt+=ttrace[i]*ttrace[i];
    if(i<fftlen/2) {
      psumf+=4*fmag[i]*fmag[i]/(fftlen*fftlen);
      HNF->Fill(i+0.5,2*fmag[i]/fftlen);
    }
  }
  HNT->Write();
  HNF->Write();
  printf("1024 Noise: Time %g Freq %g\n",psumt/fftlen,psumf/2);
  free(ttrace);
  free(fmag);
  free(fphase);
  fftlen = 8192;
  fft_init(fftlen);
  ttrace = (float *)malloc(fftlen*sizeof(float));
  fmag = (float *)malloc(fftlen*sizeof(float));
  fphase = (float *)malloc(fftlen*sizeof(float));
  for(i=0;i<fftlen;i++) ttrace[i] = 3*sin(i/8.);
  mag_and_phase(ttrace,fmag,fphase);
  TH1F *HT2 = new TH1F("HT2","Trace",fftlen,0.,fftlen);
  TH1F *HF2 = new TH1F("HF2","FFT",fftlen/2,0.,512);
  psumt = 0;
  psumf = 0;
  for(i=0;i<fftlen;i++){
    HT2->Fill(i+0.5,ttrace[i]);
    psumt+=ttrace[i]*ttrace[i];
    if(i<fftlen/2){
      psumf+=4*fmag[i]*fmag[i]/(fftlen*fftlen);
      HF2->Fill((i+0.5)*1024./fftlen,2*fmag[i]/fftlen);
    }
  }
  printf("8196 Sine: Time %g Freq %g\n",psumt/fftlen,psumf/2);
  HT2->Write();
  HF2->Write();
  for(i=0;i<fftlen;i++) ttrace[i] = 1.5*noise->Gaus();
  mag_and_phase(ttrace,fmag,fphase);
  TH1F *HNT2 = new TH1F("HNT2","Trace",fftlen,0.,fftlen);
  TH1F *HNF2 = new TH1F("HNF2","FFT",fftlen/2,0.,512);
  psumt = 0;
  psumf = 0;
  for(i=0;i<fftlen;i++){
    HNT2->Fill(i+0.5,ttrace[i]);
    psumt+=ttrace[i]*ttrace[i];
    if(i<fftlen/2){
      psumf+=4*fmag[i]*fmag[i]/(fftlen*fftlen);
      HNF2->Fill((i+0.5)*1024./fftlen,2*fmag[i]/fftlen);
    }
  }
  printf("8196 Noise: Time %g Freq %g\n",psumt/fftlen,psumf/2);
  HNT2->Write();
  HNF2->Write();
  g.Close();
}
