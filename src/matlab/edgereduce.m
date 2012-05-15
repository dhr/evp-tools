function cols = edgereduce(cols, thresh, thin)

cols = max(max(cols, [], 4), [], 3) > thresh;

if exist('thin', 'var') && thin
  cols = bwmorph(cols, 'thin', inf);
end