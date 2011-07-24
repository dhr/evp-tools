function out = evpload(filename)

s = load(filename);
out = evpconvert(s.evpout);