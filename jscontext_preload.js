// Creates class definitions and other things
var global = new Function('return this;')()

function Vector(x, y, z) {
	this.x = x
	this.y = y
	this.z = z
}

Vector.prototype.toString = function() {
	return "[" + this.x + ", " + this.y + ", " + this.z + "]"
}

global.Vector = Vector