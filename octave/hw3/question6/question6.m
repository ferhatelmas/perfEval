function question6(a, b, c, d)
  n = length(a);
  m = mean(a);
  
  x = [];
  y = [];
  for i=1:n
    x = [x, i/n];
    y = [y, sum(a(1:i))/(n*m)];
  end
  
  plot(x, y);
  title('Task 1 with lambda=50');
  hold on
  plot(x, x, 'r');
  print -f1 -r600 -depsc2 question6_1.eps;

  n = length(b);
  m = mean(b);
  
  x = [];
  y = [];
  for i=1:n
    x = [x, i/n];
    y = [y, sum(b(1:i))/(n*m)];
  end
  
  hold off
  plot(x, y);
  title('Task 2 with lambda=50');
  hold on
  plot(x, x, 'r');
  print -f1 -r600 -depsc2 question6_2.eps;
  
  n = length(c);
  m = mean(c);
  
  x = [];
  y = [];
  for i=1:n
    x = [x, i/n];
    y = [y, sum(c(1:i))/(n*m)];
  end
  
  hold off
  plot(x, y);
  title('Task 1 with lambda=70');
  hold on
  plot(x, x, 'r');
  print -f1 -r600 -depsc2 question6_3.eps;
  
  n = length(d);
  m = mean(d);
  
  x = [];
  y = [];
  for i=1:n
    x = [x, i/n];
    y = [y, sum(d(1:i))/(n*m)];
  end
  
  hold off
  plot(x, y);
  title('Task 2 with lambda=70');
  hold on
  plot(x, x, 'r');
  print -f1 -r600 -depsc2 question6_4.eps;

end