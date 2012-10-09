
load data.txt
y = data;

alpha = 0.05;

nob = 3076;

yob = y(1:nob);

d1yob = diff(yob);
d8yob = yob(8+1:length(yob))-yob(1:length(yob)-8);
d18yob = diff(d8yob);

figure(1)

plot(nob-length(d1yob)+1:nob, d1yob, 'b')
title('differencing at lag 1')


figure(2)

plot(nob-length(d8yob)+1:nob, d8yob, 'b')
title('differencing at lag 8')

figure(3)

plot(nob-length(d18yob)+1:nob, d18yob, 'b')
title('differencing at lags 1 and 8')


yfore = y;
cint = zeros(length(y)-nob, 1);
  
res=d18yob;
err=res;
  
for k=nob+1:length(y)
  yfore(k)=yfore(k-1)+yfore(k-8)-yfore(k-8-1);
  %err = yfore(nob+1:k) - y(nob+1:k);
  %res=d116yob;
  %err=res;
  % cint(k-nob) = (k-nob)*tinv(1-alpha/2, length(err)-1)*std(err)/sqrt(length(err));
  cint(k-nob) = norminv(1-alpha/2)*sqrt(k-nob)*std(err);
end

figure(4)

npast = 7;

plot(nob-npast:nob, y(nob-npast:nob), 'k')
hold on
plot(nob+1:length(y), y(nob+1:length(y)), 'o-b')
plot(nob+1:length(yfore), yfore(nob+1:length(yfore)), 'r')
plot(nob+1:length(yfore), yfore(nob+1:length(yfore))+cint, 'r-.')
plot(nob+1:length(yfore), yfore(nob+1:length(yfore))-cint, 'r-.')
hold off

for i=nob+1:length(yfore) 
  disp(i)
  disp(y(i))
  disp(yfore(i))
  disp(yfore(i)+cint(i-nob))
  disp(yfore(i)-cint(i-nob))
end 
