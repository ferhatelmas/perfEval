
function delta = getDelta(type, intensity)

mu      = 0.5;
sigma   = 2;

lower   = 1.5;
upper   = 2;

switch type
 case 'A'
  delta = exprnd(1000/intensity);
 case '1'
  delta = lognrnd(mu, sigma);
 case '2'
  delta = unifrnd(lower, upper);
end
