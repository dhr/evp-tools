function out = relaxedges(cols)

evp('progmon', 'waitbar');
h = waitbar(0, 'Relaxing Edges...');

shiftamt = [0 0 size(cols, 3)/4 0];
shiftedcols = circshift(cols, shiftamt);
out = evp('relaxedges', single(shiftedcols));
out = circshift(out, -shiftamt);

close(h);