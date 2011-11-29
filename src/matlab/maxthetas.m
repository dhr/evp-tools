function [thetas confs] = maxthetas(cols)

cols = max(max(cols, [], 5), [], 4);
[confs thetas] = max(cols, [], 3);
nthetas = size(cols, 3);
thetas = (thetas - 1)/nthetas*pi;