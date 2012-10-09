% Milan Vojnovic, May 21, 2003
% Slavisa Sarafijanovic, May 2004 
% Alaeddine El Fawal, April 2005

load sprint250.txt
y = sprint250;

alpha = 0.05;

nob = 224;

yob = y(1:nob);

d1yob = diff(yob);
d16yob = yob(16+1:length(yob))-yob(1:length(yob)-16);
d116yob = diff(d16yob);

figure(1)

plot(nob-length(d1yob)+1:nob, d1yob, 'b')
title('differencing at lag 1')


figure(2)

plot(nob-length(d16yob)+1:nob, d16yob, 'b')
title('differencing at lag 16')

figure(3)

plot(nob-length(d116yob)+1:nob, d116yob, 'b')
title('differencing at lags 1 and 16')


yfore = y;
cint = zeros(length(y)-nob, 1);
  
res=d116yob;
err=res;
  
for k=nob+1:length(y)
  yfore(k)=yfore(k-1)+yfore(k-16)-yfore(k-16-1);
  %err = yfore(nob+1:k) - y(nob+1:k);
  %res=d116yob;
  %err=res;
  % cint(k-nob) = (k-nob)*tinv(1-alpha/2, length(err)-1)*std(err)/sqrt(length(err));
  cint(k-nob) = norminv(1-alpha/2)*sqrt(k-nob)*std(err);
end

figure(4)

npast = 16;

plot(nob-npast:nob, y(nob-npast:nob), 'k')
hold on
plot(nob+1:length(y), y(nob+1:length(y)), 'o-b')
plot(nob+1:length(yfore), yfore(nob+1:length(yfore)), 'r')
plot(nob+1:length(yfore), yfore(nob+1:length(yfore))+cint, 'r-.')
plot(nob+1:length(yfore), yfore(nob+1:length(yfore))-cint, 'r-.')
hold off
