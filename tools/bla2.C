{
  TCanvas *c = new TCanvas();
  c->Divide(2,2);
  c->cd(1);
  HNT->Draw("HIST");
  c->cd(2);
  HNT2->Draw("HIST");
  c->cd(3);
  HNF->Draw("HIST");
  c->cd(4);
  HNF2->Draw("HIST");
}
