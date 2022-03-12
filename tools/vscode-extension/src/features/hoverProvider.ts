
import { HoverProvider, Hover, MarkedString, TextDocument, CancellationToken, Position, workspace, window } from 'vscode';
import { textToMarkedString } from './utils/markedTextUtil';
import eviSymbols = require('./eviSymbols');

const constantRegexes: { [regex: string]: string } = {
	/*integers*/ "^(-?(?:0(x|X)[0-f]+)|(0(c|C)[0-7]+)|(0(b|B)[0-1]+)|([0-9]+))$": "i32",
	/*floats  */ "^(-?[0-9]+(\.[0-9]+)?)$": "dbl",
	/*strings */ "^(\"(\\.|.)*\")$": "chr*",
}

export default class EviHoverProvider implements HoverProvider {

	public provideHover(document: TextDocument, position: Position, _token: CancellationToken): Hover | undefined {
		let wordRange = document.getWordRangeAtPosition(position);
		let word = wordRange ? document.getText(wordRange) : '';
		
		if (eviSymbols.keywords[word]) {
			// its a keyword
			let entry = eviSymbols.keywords[word];
			let signature = { language: 'evi', value: entry.signature || word };
			let documentation = textToMarkedString(entry.description || '');
			return new Hover([signature, documentation], wordRange);
		}
		else if (eviSymbols.types[word])
		{
			// its a type
			let documentation = textToMarkedString(eviSymbols.types[word].description);
			return new Hover([{
				language: 'evi',
				value: word
			}, documentation], wordRange);
		}
		else if(word.startsWith('#'))
		{
			// preprocessor directive
			for (let directive in eviSymbols.directives) {
				if (word.substring(1) === directive)
				{
					let entry = eviSymbols.directives[directive];
					let signature = { language: 'evi', value: entry.signature || word };
					let documentation = textToMarkedString(entry.description || '');
					return new Hover([signature, documentation], wordRange);
				}
			}
		}
		for (let regex in constantRegexes) {
			if(new RegExp(regex).test(word))
				return new Hover({ 
					language: 'evi', 
					value: word + ' -> ' + constantRegexes[regex]
				}, wordRange);
		}

		return wordRange ? new Hover([{ language: 'evi', value: word }, "Symbol not found."], wordRange) : undefined;
	}
}