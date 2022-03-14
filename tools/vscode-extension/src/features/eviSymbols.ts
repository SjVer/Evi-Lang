export interface IEntry { description?: string; signature?: string }
export interface IEntries { [name: string]: IEntry }

export const keywords: IEntries = {
	'@': {
		description: 'declare/define function',
		signature: '@func type (params)'
	},
	'%': {
		description: 'declare/define variable',
		signature: '%var type value'	
	},
	'~': {
		description: 'return',
		signature: '~ [value]'
	},
	'=': {
		description: 'assign',
		signature: '=var value'
	},
	'??': {
		description: 'if',
		signature: '??(cond) then [:: else]'
	},
	'::': {
		description: 'else',
		signature: '??(cond) then [:: else]'
	},
	'!!': {
		description: 'loop',
		signature: '!!([init]; [cond]; [inc];) body'	
	},
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
	'i1': {
		description: "1-bit signed integer",
		signature: "1-bit signed int"
	},
	
	'i4': {
		description: "4-bit signed integer",
		signature: "4-bit signed int"
	},
	'ui4': {
		description: "4-bit unsigned integer",
		signature: "4-bit unsigned int"
	},
	
	'i8': {
		description: "8-bit signed integer",
		signature: "8-bit signed int"
	},
	'ui8': {
		description: "8-bit unsigned integer",
		signature: "8-bit unsigned int"
	},
	
	'i16': {
		description: "16-bit signed integer",
		signature: "16-bit signed int"
	},
	'ui16': {
		description: "16-bit unsigned integer",
		signature: "16-bit unsigned int"
	},
	
	'i32': {
		description: "32-bit signed integer",
		signature: "32-bit signed int"
	},
	'ui32': {
		description: "32-bit unsigned integer",
		signature: "32-bit unsigned int"
	},
	
	'i64': {
		description: "64-bit signed integer",
		signature: "64-bit signed int"
	},
	'ui64': {
		description: "64-bit unsigned integer",
		signature: "64-bit unsigned int"
	},
	
	'i128': {
		description: "128-bit signed integer",
		signature: "128-bit signed int"
	},
	'ui128': {
		description: "128-bit unsigned integer",
		signature: "128-bit unsigned int"
	},
	
	'flt': {
		description: "32-bit single precision floating point number",
		signature: "single precision float"
	},
	'dbl': {
		description: "64-bit double precision floating point number",
		signature: "double precision float"
	},
	
	'bln': {
		description: "unsigned 1-bit boolean",
		signature: "boolean"
	},
	'chr': {
		description: "unsigned 8-bit character",
		signature: "8-bit character"
	},
	'nll': {
		description: "null/void type",
		signature: "null/void"
	},
}