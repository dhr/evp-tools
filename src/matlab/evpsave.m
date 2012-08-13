function evpsave(file, data)

evpdata = permute(flipdim(single(data), 1), [2 1 3:ndims(data)]); %#ok<NASGU>
save(file, 'evpdata');
