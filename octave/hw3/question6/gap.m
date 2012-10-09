function gap = gap(in) 
  m = mean(in);
  a = mean(abs(in-m));
  gap = a / (2*m);
end