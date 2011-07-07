function out = relaxlines(cols)

evp('progmon', 'waitbar');
h = waitbar(0, 'Relaxing Edges...');

shiftamt = [0 0 size(cols, 3)/4 0];
shiftedcols = circshift(cols, shiftamt);
out = evp('relaxlines', single(shiftedcols));
out = circshift(out, -shiftamt);

close(h);