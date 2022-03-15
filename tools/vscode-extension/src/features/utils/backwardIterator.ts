import { TextDocument } from "vscode";

// export enum CharCodes {
// 	NL = '\n'.charCodeAt(0),
// 	TAB = '\t'.charCodeAt(0),
// 	WSB = ' '.charCodeAt(0),
// 	LBracket = '['.charCodeAt(0),
// 	RBracket = ']'.charCodeAt(0),
// 	LCurly = '{'.charCodeAt(0),
// 	RCurly = '}'.charCodeAt(0),
// 	LParent = '('.charCodeAt(0),
// 	RParent = ')'.charCodeAt(0),
// 	Comma = ','.charCodeAt(0),
// 	Quote = '\''.charCodeAt(0),
// 	DQuote = '"'.charCodeAt(0),
// 	USC = '_'.charCodeAt(0),
// 	a = 'a'.charCodeAt(0),
// 	z = 'z'.charCodeAt(0),
// 	A = 'A'.charCodeAt(0),
// 	Z = 'Z'.charCodeAt(0),
// 	_0 = '0'.charCodeAt(0),
// 	_9 = '9'.charCodeAt(0),
// 	BOF = 0
// }

export class BackwardIterator {
	private lineNumber: number;
	private offset: number;
	private line: string;
	private model: TextDocument;

	constructor(model: TextDocument, offset: number, lineNumber: number) {
		this.lineNumber = lineNumber;
		this.offset = offset;
		this.line = model.lineAt(this.lineNumber).text;
		this.model = model;
	}

	public hasNext(): boolean {
		return this.lineNumber >= 0;
	}

	public next(): string {
		if (this.offset < 0) {
			if (this.lineNumber > 0) {
				this.lineNumber--;
				this.line = this.model.lineAt(this.lineNumber).text;
				this.offset = this.line.length - 1;
				// return CharCodes.NL;
				return '\n';
			}
			this.lineNumber = -1;
			// return CharCodes.BOF;
			return '\0';
		}
		// let ch = this.line.charCodeAt(this.offset);
		let ch = this.line.charAt(this.offset);
		this.offset--;
		return ch;
	}

}