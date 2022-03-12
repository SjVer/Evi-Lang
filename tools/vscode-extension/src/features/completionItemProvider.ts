import { CompletionItemProvider, CompletionItem, CompletionItemKind, CancellationToken, TextDocument, Position, Range, TextEdit, workspace, CompletionContext, window } from 'vscode';
import eviSymbols = require('./eviSymbols');

export default class EviCompletionItemProvider implements CompletionItemProvider {

	public provideCompletionItems(document: TextDocument, position: Position, _token: CancellationToken, context: CompletionContext): Promise<CompletionItem[]> {
		let result: CompletionItem[] = [];

		let shouldProvideCompletionItems = workspace.getConfiguration('evi').get<boolean>('suggestions', true);
		if (!shouldProvideCompletionItems) return Promise.resolve(result);

		let range = document.getWordRangeAtPosition(position);
		let prefix = range ? document.getText(range) : '';
		if (!range) range = new Range(position, position);


		let added: any = {};
		let createNewProposal = function (kind: CompletionItemKind, name: string, entry: eviSymbols.IEntry | null): CompletionItem {
			let proposal: CompletionItem = new CompletionItem(name);
			proposal.kind = kind;
			if (entry) {
				if (entry.description) proposal.documentation = entry.description;
				if (entry.signature) proposal.detail = entry.signature;
			}
			return proposal;
		};
		// let matches = (name: string) => { return prefix.length === 0 || name.length >= prefix.length && name.substr(0, prefix.length) === prefix; };
		let matches = (name: string) => { return prefix.length === 0 || name.startsWith(prefix); };


		// search for keywords
		for (let keyword in eviSymbols.keywords) {
			if (matches(keyword)) {
				added[keyword] = true;
				result.push(createNewProposal(CompletionItemKind.Keyword, keyword, eviSymbols.keywords[keyword]));
			}
		}

		// search for variables
		if (prefix[0] === '$') {
			let text = document.getText(new Range(new Position(0, 0), range.start));
			let variableMatch = /(?:\$|=|\%)([A-z_]+[a-zA-Z0-9_]*)/g;
			let parameterMatch = /\$([0-9]+)/g;

			// check variables
			if(variableMatch.test(prefix) || prefix.length === 1)
			{
				let match: RegExpExecArray | null = null;
				while (match = variableMatch.exec(text)) {
					let word = '$' + match[1];
					if (!added[word]) {
						added[word] = true;
						result.push(createNewProposal(CompletionItemKind.Variable, word, null));
					}
				}
			}
			// check parameters
			if(parameterMatch.test(prefix) || prefix.length === 1)
			{

			}

		}

		// search for types
		// else if (eviSymbols.types[prefix])

		// search in functions
		else {
			let text = document.getText(new Range(new Position(0, 0), range.start));
			let identMatch = /([A-z_]+[a-zA-Z0-9_]*)/g;
			let declMatch = /@\s*([A-z_]+[a-zA-Z0-9_]*)/g;
			let callMatch = /([A-z_]+[a-zA-Z0-9_]*)\s*\(/g;

			// check variables
			if(identMatch.test(prefix) || prefix.length === 1)
			{
				let match: RegExpExecArray | null = null;
				while (match = declMatch.exec(text) || callMatch.exec(text)) {
					let word = match[1];
					if (!added[word]) {
						added[word] = true;
						result.push(createNewProposal(CompletionItemKind.Function, word, null));
					}
				}
			}
		}

		return Promise.resolve(result);
	}
}