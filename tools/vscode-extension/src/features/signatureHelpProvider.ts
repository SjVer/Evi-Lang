import { SignatureHelpProvider, SignatureHelp, SignatureInformation, CancellationToken, TextDocument, Position, workspace, window } from 'vscode';
import { callEviLint, eviLintType, getDocumentation } from './utils/eviLintUtil';
import { BackwardIterator } from './utils/backwardIterator';

export default class EviSignatureHelpProvider implements SignatureHelpProvider {

	public async provideSignatureHelp(document: TextDocument, position: Position, _token: CancellationToken): Promise<SignatureHelp | null> {
		if (!workspace.getConfiguration('evi').get<boolean>('suggestions', true)) return null;

		let iterator = new BackwardIterator(document, position.character - 1, position.line);

		let paramCount = this.readArguments(iterator);
		if (paramCount < 0) return null;

		let ident = this.readIdent(iterator);
		if (!ident) return null;

		// find params and whatnot
		let signature: string = "";
		let params: string[] = [];
		callEviLint(document, eviLintType.getFunctions, position).functions.forEach(func => {
			if(signature.length || ident != func.identifier) return;

			signature = `@${func.identifier} ${func.return_type} (`;
			for (let param in func.parameters) {
				params.push(func.parameters[param]);
				signature += func.parameters[param] + ' ';
			}
			if (signature.endsWith(' ')) signature = signature.substring(0, signature.length - 1);
			signature += ')';
		});
		if(!signature.length) return Promise.reject("Function not found.");

		const doc = await getDocumentation(document, position);
		let signatureInfo = new SignatureInformation(signature, doc);
		params.forEach(param => signatureInfo.parameters.push({ label: param, documentation: undefined }));
		
		let ret = new SignatureHelp();
		ret.signatures.push(signatureInfo);
		ret.activeSignature = 0;
		ret.activeParameter = Math.min(paramCount, signatureInfo.parameters.length - 1);
		return Promise.resolve(ret);
	}

	private readArguments(iterator: BackwardIterator): number {
		let parentNesting = 0;
		let bracketNesting = 0;
		let curlyNesting = 0;
		let paramCount = 0;
		while (iterator.hasNext()) {
			let ch = iterator.next();
			switch (ch) {
				// case CharCodes.LParent:
				case '(':
					parentNesting--;
					if (parentNesting < 0) {
						return paramCount;
					}
					break;
				// case CharCodes.RParent: parentNesting++; break;
				case ')': parentNesting++; break;
				// case CharCodes.LCurly: curlyNesting--; break;
				case '{': curlyNesting--; break;
				// case CharCodes.RCurly: curlyNesting++; break;
				case '}': curlyNesting++; break;
				// case CharCodes.LBracket: bracketNesting--; break;
				case '[': bracketNesting--; break;
				// case CharCodes.RBracket: bracketNesting++; break;
				case ']': bracketNesting++; break;
				// case CharCodes.DQuote:
				case '"':
				// case CharCodes.Quote:
				case '\'':
					while (iterator.hasNext() && ch !== iterator.next()) {
						// find the closing quote or double quote
					}
					break;
				// case CharCodes.Comma:
				case ',':
					if (!parentNesting && !bracketNesting && !curlyNesting) paramCount++;
					break;
			}
		}
		return -1;
	}

	private isIdentPart(ch: string): boolean {
		// if (ch === CharCodes.USC || // _
		// 	ch >= CharCodes.a && ch <= CharCodes.z || // a-z
		// 	ch >= CharCodes.A && ch <= CharCodes.Z || // A-Z
		// 	ch >= CharCodes._0 && ch <= CharCodes._9 || // 0/9
		// 	ch >= 0x80 && ch <= 0xFFFF) { // nonascii
		return /[a-zA-Z0-9_]/.test(ch);
	}

	private readIdent(iterator: BackwardIterator): string {
		let identStarted = false;
		let ident = '';
		while (iterator.hasNext()) {
			let ch = iterator.next();
			if (!identStarted && /\s/.test(ch)) { // (ch === CharCodes.WSB || ch === CharCodes.TAB || ch === CharCodes.NL)) {
				continue;
			}
			if (this.isIdentPart(ch)) {
				identStarted = true;
				// ident = String.fromCharCode(ch) + ident;
				ident = ch + ident;
			} else if (identStarted) {
				return ident;
			}
		}
		return ident;
	}
}