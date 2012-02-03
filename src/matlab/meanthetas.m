function [thetas confs] = meanthetas(cols)

cols = max(max(cols, [], 5), [], 4);
totals = sum(cols, 3);
nthetas = size(cols, 3);
anglevals = repmat(shiftdim(0:nthetas - 1, -1), size(totals))*2*pi/nthetas;
normed = bsxfun(@rdivide, cols, totals);
normed(totals == 0) = 0;
thetas = atan2(sum(normed.*sin(anglevals), 3), sum(normed.*cos(anglevals), 3))/2;
confs = max(cols, [], 3);
thetas(confs == 0) = 0;