import { CompletionItemProvider, CompletionItem, CompletionItemKind, CancellationToken, TextDocument, 
		Position, Range, workspace, CompletionContext, window } from 'vscode';
import eviSymbols = require('./eviSymbols');
import { callEviLint, eviLintType, eviLintResult } from './utils/eviLintUtil';

const identifierRegex: RegExp = /([A-z_]+[a-zA-Z0-9_]*)/g;

export default class EviCompletionItemProvider implements CompletionItemProvider {

	public provideCompletionItems(document: TextDocument, position: Position, _token: CancellationToken, context: CompletionContext): Promise<CompletionItem[]> {
		let result: CompletionItem[] = [];
		let added: any = {};

		let shouldProvideCompletionItems = workspace.getConfiguration('evi').get<boolean>('suggestions', true);
		if (!shouldProvideCompletionItems) return Promise.resolve(result);

		let range = document.getWordRangeAtPosition(position);
		let prefix = range ? document.getText(range) : '';
		if (!range) range = new Range(position, position);

		let createNewProposal = function (kind: CompletionItemKind, name: string, entry: eviSymbols.IEntry | null): CompletionItem {
			let proposal: CompletionItem = new CompletionItem(name);
			proposal.kind = kind;
			if (entry) {
				if (entry.description) proposal.documentation = entry.description;
				if (entry.signature) proposal.detail = entry.signature;
			}
			return proposal;
		};
		let matches = (name: string) => { return prefix.length === 0 || name.length >= prefix.length && name.substring(0, prefix.length) === prefix; };
		// let getTabCountBeforePosition = function (start: Position): number {
		// 	let range = new Range(start, start);
		// 	while (range.isSingleLine) range = new Range(range.start.translate(0, -1), start);
		// 	const text = document.getText(range);
		// 	window.showInformationMessage(`"${text}"`);

		// 	let tabc: number = 0;
		// 	for (let i = 0; i < text.length; i++) if (text.charAt(i) == '\t') tabc++;
		// 	return tabc;
		// }

		// search for keywords
		for (let keyword in eviSymbols.keywords) {
			if (matches(keyword)) {
				added[keyword] = true;
				result.push(createNewProposal(CompletionItemKind.Keyword, keyword, eviSymbols.keywords[keyword]));
			}
		}

		// search for variables
		if (prefix[0] === '$') {
			const vars: eviLintResult = callEviLint(document, eviLintType.getVariables, position);

			for(var variable of vars.elements) {
				let word = '$' + variable.identifier;
				if (!added[word]) {
					added[word] = true;
					const signature = `$${variable.identifier} -> ${variable.properties}`;
					result.push(createNewProposal(CompletionItemKind.Variable, word, { signature: signature }));
				}
			}
		}

		// search for types
		for (let type in eviSymbols.types) {
			if (matches(type)) {
				added[type] = true;
				result.push(createNewProposal(CompletionItemKind.Class, type, eviSymbols.types[type]));
			}
		}

		// search for directives
		if (prefix[0] === '#')
		{
			for (let directive in eviSymbols.directives) {
				if (matches('#' + directive)) {
					added['#' + directive] = true;
					result.push(createNewProposal(CompletionItemKind.Keyword, '#' + directive, eviSymbols.types[directive]));
				}
			}
		}

		// search in functions
		if (identifierRegex.test(prefix)) {
			const funcs: eviLintResult = callEviLint(document, eviLintType.getFunctions, position);

			for(var func of funcs.elements) {
				let word = func.identifier;
				if (!added[word]) {
					added[word] = true;
					const signature = `@${func.identifier} ${func.properties}`;
					result.push(createNewProposal(CompletionItemKind.Function, word, { signature: signature }));
				}
			}
		}

		return Promise.resolve(result);
	}
}