function out = evpload(filename)

s = load(filename);
out = double(flipdim(permute(s.evpout, [2 1 3:ndims(in)]), 1));
