export interface IEntry { description?: string; signature?: string }
export interface IEntries { [name: string]: IEntry }

export const keywords: IEntries = {
	'@': { description: 'declare/define function' },
	'%': { description: 'declare/define variable' },
	'~': { description: 'return' },
	'=': { description: 'assign' },
	'??': { description: 'if' },
	'::': { description: 'else' },
	'!!': { description: 'loop' },
};

export const directives: IEntries = {
	'apply': {
		description: "import directive",
		signature: "#apply \"FILENAME\""
	},
	'info': {
		description: "pragma directive",
		signature: "#info OPTION \\ [ARGUMENTS...] (compiler specific)"
	},
	'file': {
		description: "filename directive",
		signature: "#file \"FILENAME\""
	},
	'line': {
		description: "line number directive",
		signature: "#line LINENO"
	},

	'flag': {
		description: "set flag directive",
		signature: "#flag FLAG"
	},
	'unflag': {
		description: "unset flag directive",
		signature: "#unflag FLAG"
	},

	'ifset': {
		description: "if-flag-is-set directive",
		signature: "#ifset FLAG"
	},
	'ifnset': {
		description: "if-flag-is-not-set directive",
		signature: "#ifnset FLAG"
	},
	'else': {
		description: "else directive",
		signature: "#else"
	},
	'endif': {
		description: "end if-statement directive",
		signature: "#endif"
	},
};

export const types: IEntries = {
	'i1': { description: "1-bit signed integer" },

	'i4': { description: "4-bit signed integer" },
	'ui4': { description: "4-bit unsigned integer" },

	'i8': { description: "8-bit signed integer" },
	'ui8': { description: "8-bit unsigned integer" },

	'i16': { description: "16-bit signed integer" },
	'ui16': { description: "16-bit unsigned integer" },

	'i32': { description: "32-bit signed integer" },
	'ui32': { description: "32-bit unsigned integer" },

	'i64': { description: "64-bit signed integer" },
	'ui64': { description: "64-bit unsigned integer" },

	'i128': { description: "128-bit signed integer" },
	'ui128': { description: "128-bit unsigned integer" },

	'flt': { description: "32-bit single precision floating point number" },
	'dbl': { description: "64-bit double precision floating point number" },
	
	'bln': { description: "unsigned 1-bit boolean" },
	'chr': { description: "unsigned 8-bit character" },
	'nll': { description: "null/void type" },
}