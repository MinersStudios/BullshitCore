import { Core } from './Core.js'

export function BullshitCore() {
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

BullshitCore.prototype.getName = function() {
	return "BullshitCore"
}
