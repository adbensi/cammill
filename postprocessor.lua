require("math")

post = {}
post.Text = function (...)
	out = string.format("%s", select(1,...))
	append_output(out)
end

post.ModalText = function (str)
	append_output(str)
end

post.Number = function (val, format)
	out = string.format("%0.3f", val)
	append_output(out)
end

post.ModalNumber = function (str, val, format)
	out = string.format("%s%0.3f", str, val)
	append_output(out)
end

post.NonModalNumber = function (str, val, format)
	out = string.format("%s%0.3f", str, val)
	append_output(out)
end

post.Eol = function ()
	append_output("\n")
end

post.Warning = function (str)
	io.write("(WARNING: ", str, ")\n")
end

post.SetCommentChars = function (str)
--	io.write("(SetCommentChars: ", str, ")\n")
end

post.ForceExtension = function (str)
	io.write("(force EXTENSION: ", str, ")\n")
end

post.ArcAsMoves = function (str)
end


event = {}
function event:GetTextCtrl ()
	local res = o or {}
	setmetatable(res, self)
	self.__index = self
	return res
end

function event:AppendText (str)
	io.write(str)
end

function load_post ()
	dofile(postfile)
end






