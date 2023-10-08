load("src/main/javascript/Core.js")

function BullshitCore() {
	Core.call(this)
}
BullshitCore.prototype = Object.create(Core.prototype, {
	constructor: {
		value: BullshitCore,
		enumerable: false,
		writable: true,
		configurable: true
	}
})

/**
 * @override
 */
BullshitCore.prototype.getName = function() {
	return "BullshitCore"
}
