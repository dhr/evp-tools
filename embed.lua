local function escape(line)
  line = line:gsub("\\", "\\\\") -- backslashes
  line = line:gsub("\n", "\\n") -- newlines
  line = line:gsub("\"", "\\\"") -- quotes
  return line
end

function doembed()
  local files = os.matchfiles("**.cl")
  for i, infile in ipairs(files) do
    local outfile = infile:gsub("(.+)\.cl$", "%1.clstr")
    print(infile .. " --> " .. outfile)
    local out = io.open(outfile, "w+b")
    for line in io.lines(infile) do
      out:write("\"")
      out:write(escape(line))
      out:write("\\n\"\n")
    end
    out:close()
  end
end
