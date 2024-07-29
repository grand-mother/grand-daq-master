{
  TCanvas *c = new TCanvas();
  c->Divide(2,2);
  c->cd(1);
  HT->Draw("HIST");
  c->cd(2);
  HT2->Draw("HIST");
  c->cd(3);
  HF->Draw("HIST");
  c->cd(4);
  HF2->Draw("HIST");
}
