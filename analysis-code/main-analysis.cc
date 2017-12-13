#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <sstream>
#include "TH2.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TROOT.h"
#include "TH1F.h"
#include "TChain.h"
#include "TFile.h"
#include "TClonesArray.h"
#include "TObject.h"
#include "classes/DelphesClasses.h"
#include "fastjet-analysis.cc"
#include "mpi.h"
#define MASTER_MPI 0
namespace dataevents {
#include "event.C"
}

void write_to_file(std::vector<float> e2_main);
void add_to_hist(std::vector<float> e2_main,TH1 *e2_hist);
void hist_to_file(TH1 *e2,double *a);
void eflow_analysis(Float_t *ET,Float_t *eta,Float_t *phi,Float_t *E,Int_t size,std::vector<float> &e2,double *a);
template <typename T>
void EventLoop(T &event,std::vector<float> &e2,double *a,int &taskid,int &numtasks)
{
   if (event.fChain == 0) return;
   Long64_t nentries = event.fChain->GetEntriesFast();
   MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
   Long64_t chunksize = nentries/numtasks;
   //Care is not taken to take all the events. The events are equally distributed to the processes and the left over is discarded.
   //So if no process should be discarded then the number of processes should be a divisor of the number of events
   for (Long64_t k = 0; k<chunksize;k++) {
     Long64_t jentry = (taskid*chunksize) +  k ;
     if(jentry%100 == 0){
     std::cout<<std::setw(15)<<"Gathering event  "<<std::setw(10)<<jentry<<std::setw(15)<<
         " in the process "<<std::setw(2)<<taskid<<std::setw(5)<<"("<<(k*100.0)/chunksize<<"% )"<<std::endl;}
     event.GetEntry(jentry);
     root_to_fastjet(event.Particle_Px,event.Particle_Py,event.Particle_Pz,event.Particle_E,event.Particle_Status,event.Particle_PID,event.Particle_size,e2,a);
     //eflow_analysis(event.eflow_ET,event.eflow_Eta,event.eflow_Phi,event.eflow_E,event.eflow_size,e2,a); 
   }

}
void eflow_analysis(Float_t *ET,Float_t *eta,Float_t *phi,Float_t *E,Int_t size,std::vector<float> &e2,double *a){
  Float_t px[size];
  Float_t py[size];
  Float_t pz[size];
  Int_t status[size];
  Int_t PID[size];
  for(int i=0;i<size;i++){
    px[i] = ET[i]*std::cos(phi[i]);
    py[i] = ET[i]*std::sin(phi[i]);
    pz[i] = ET[i]*std::sinh(eta[i]);
    status[i] = 1;
    PID[i] = 0;
  }
   root_to_fastjet(px,py,pz,E,status,PID,size,e2,a);

}

//Main Function
int main(){
   MPI_Init(NULL,NULL);
   int taskid,numtasks;
   MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
   MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
     std::ifstream fjetin("settings-fastjet.txt");
     double a[4];
     fjetin>>a[0];
     fjetin>>a[1];
     fjetin>>a[2];
     fjetin>>a[3];
   if(taskid == MASTER_MPI){
     std::ofstream f("images.txt");
     f.close();
     std::cout<<"SETTINGS:   "<<std::endl<<
       "Alpha   :"<<a[0]<<std::endl<<
       "Z cut    :"<<a[1]<<std::endl<<
       "Beta    :"<<a[2]<<std::endl<<
       "Pt Min   :"<<a[3]<<std::endl;
   std::cout<<"Number of tasks   "<<numtasks<<std::endl;
   }
   std::vector<float> e2;
  dataevents::event t;
  EventLoop(t,e2,a,taskid,numtasks);
  if(taskid != MASTER_MPI){
    std::cout<<"Outgoing Size  :"<<e2.size()<<"From   :"<<taskid<<std::endl;
    MPI_Send(&e2.front(),e2.size(),MPI_FLOAT,MASTER_MPI,0,MPI_COMM_WORLD);
  }
  if(taskid == MASTER_MPI){
     std::vector<float> e2_main;
     e2_main = e2;
     for(int k=1;k<numtasks;k++){
       std::vector<float> e2_temp;
       int incoming_size;
       MPI_Status status;
       MPI_Probe(k,0,MPI_COMM_WORLD,&status);
       MPI_Get_count(&status,MPI_FLOAT,&incoming_size);
       e2_temp.resize(incoming_size);
       MPI_Recv(&e2_temp.front(),incoming_size,MPI_FLOAT,k,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
       e2_main.insert(e2_main.end(),e2_temp.begin(),e2_temp.end());
     }
     std::cout<<"Size of main    :"<<e2_main.size()<<std::endl;
     std::cout<<"Max Size of main    :"<<e2_main.max_size()<<std::endl;
     //TH1 *e2_hist = new TH1F("hist","hist",50,0,14);
     //add_to_hist(e2_main,e2_hist);
     //hist_to_file(e2_hist,a);
     write_to_file(e2_main);
  } 
  MPI_Finalize();
}
void write_to_file(std::vector<float> e2_main){
  std::ofstream file;
  file.open("data.txt");
    for(int i=0;i<e2_main.size();i++){
    if(i%3 == 0) file<<std::endl;
    file<<e2_main[i]<<"\t";
    }
  std::cout<<"Written to file"<<std::endl;
}
void add_to_hist(std::vector<float> e2_main,TH1 *e2_hist){
     for(int n=0;n<e2_main.size();n++){
       e2_hist->Fill(e2_main[n]);
     }
}


void hist_to_file(TH1 *e2_hist,double *a){
     std::ostringstream filename;
     filename<<"histos_"<<a[0]<<"_"<<a[1]<<"_"<<a[2]<<"_"<<a[3]<<".root";
     std::string file;
     file = filename.str();
     //TFile *f = new TFile(file.c_str(),"RECREATE");
      TFile f(file.c_str(),"RECREATE");
 //    TCanvas *c1= new TCanvas("c1","e2",600,400);
//     e2_hist->Draw();
  //   f->WriteTObject(c1,"e2_hist");
    e2_hist->Write();
}
