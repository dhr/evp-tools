function [thetas confs] = meanthetas(cols)

cols = max(max(cols, [], 5), [], 4);
totals = sum(cols, 3);
nthetas = size(cols, 3);
anglevals = repmat(shiftdim(0:nthetas - 1, -1), size(totals))/nthetas*pi;
thetas = sum(bsxfun(@rdivide, cols, totals).*anglevals, 3);
confs = max(cols, [], 3);