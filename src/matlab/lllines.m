function cols = lllines(image)

evp('progmon', 'waitbar');
h = waitbar(0, 'Logical/Linear...');

out = evp('lllines', single(image));
cols = circshift(out, [0 0 -size(out, 3)/4 0]);

close(h);