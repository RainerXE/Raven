-- Raven for LuaJIT: .xmllua files, an interface in Raven XML with its Lua code.
--
--   local xmllua = require("raven_xmllua")   -- once: .xmllua files can then be required
--   require("main")                          -- main.xmllua, found on package.path
--   xmllua.run("main.xmllua", ...)           -- or runs one as the program
--
--   luajit raven_xmllua.lua main.xmllua      -- the same, from the command line
--
-- ravenc turns the file into Lua (the interface becomes a table: no XML is
-- read when it runs) next to it, as main.xmllua.lua. ravenc is RAVENC, or
-- found in the PATH. The functions named in {} are global functions, or
-- fields of a global table (App.on_ok).

local M = {}

local function quote(text)
	return '"' .. tostring(text):gsub('"', '\\"') .. '"'
end

function M.ravenc()
	return os.getenv("RAVENC") or "ravenc"
end

-- The Lua file made from an .xmllua.
function M.compile(path)
	local output = path .. ".lua"
	local command = quote(M.ravenc()) .. " " .. quote(path) .. " -o " .. quote(output) .. " --lang lua"
	if package.config:sub(1, 1) == "\\" then command = '"' .. command .. '"' end
	local status = os.execute(command)
	if status ~= 0 and status ~= true then
		error("ravenc failed on " .. path .. " (is RAVENC the ravenc program?)", 2)
	end
	return output
end

-- Runs an .xmllua, with these arguments in `...`.
function M.run(path, ...)
	local chunk = assert(loadfile(M.compile(path)))
	return chunk(...)
end

-- require("name") finds name.xmllua where it would find name.lua
local function searcher(name)
	local path = package.path:gsub("%?%.lua", "?.xmllua")
	local file, message
	if package.searchpath then
		file, message = package.searchpath(name, path)
	else
		for template in path:gmatch("[^;]+") do
			local candidate = template:gsub("%?", (name:gsub("%.", "/")))
			local handle = io.open(candidate, "r")
			if handle then handle:close() file = candidate break end
		end
	end
	if not file then return message or ("\n\tno file '" .. name .. ".xmllua'") end
	return assert(loadfile(M.compile(file))), file
end

local searchers = package.searchers or package.loaders
local installed = false
for _, existing in ipairs(searchers) do if existing == searcher then installed = true end end
if not installed then table.insert(searchers, 2, searcher) end

-- luajit raven_xmllua.lua file.xmllua [arguments]
if arg and arg[0] and arg[0]:match("raven_xmllua%.lua$") and arg[1] then
	M.run(arg[1], select(2, unpack(arg)))
end

return M
