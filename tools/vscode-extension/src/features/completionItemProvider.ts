import { CompletionItemProvider, CompletionItem, CompletionItemKind, CancellationToken, TextDocument, 
		Position, Range, workspace, CompletionContext, window } from 'vscode';
import eviSymbols = require('./eviSymbols');
import { callEviLint, eviLintType, eviLintFunctions, eviLintVariables } from './utils/eviLintUtil';

const identifierRegex: RegExp = /^[a-zA-Z_][a-zA-Z0-9_]*$/g;

export default class EviCompletionItemProvider implements CompletionItemProvider {

	public async provideCompletionItems(document: TextDocument, position: Position, _token: CancellationToken, context: CompletionContext): Promise<CompletionItem[]> {
		let result: CompletionItem[] = [];
		let added: any = {};

		let shouldProvideCompletionItems = workspace.getConfiguration('editor').get<boolean>('acceptSuggestionOnCommitCharacter', true);
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

		// search for keywords
		for (let keyword in eviSymbols.keywords) {
			if (matches(keyword)) {
				added[keyword] = true;
				result.push(createNewProposal(CompletionItemKind.Keyword, keyword, eviSymbols.keywords[keyword]));
			}
		}

		// search for variables
		if (prefix[0] === '$' || prefix.length === 0) {
			const vars: eviLintVariables = await callEviLint(document, eviLintType.getVariables, position);

			for(var variable of vars.variables) {
				let word = '$' + variable.identifier;
				if (matches(word) && !added[word]) {
					added[word] = true;
					const signature = `$${variable.identifier} -> ${variable.type}`;
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
		if (prefix[0] === '#' || prefix.length === 0)
		{
			for (let directive in eviSymbols.directives) {
				if (matches('#' + directive)) {
					added['#' + directive] = true;
					result.push(createNewProposal(CompletionItemKind.Keyword, '#' + directive, eviSymbols.directives[directive]));
				}
			}
		}

		// search in functions
		if (identifierRegex.test(prefix) || prefix.length === 0) {
			const funcs: eviLintFunctions = await callEviLint(document, eviLintType.getFunctions, position);

			for(var func of funcs.functions) {
				let word = func.identifier;
				if (matches(word) && !added[word]) {
					added[word] = true;

					let signature = `@${func.identifier} ${func.return_type} (`;
					func.parameters.forEach(param => signature += param + ' ');
					if(func.variadic) signature += '... ';
					signature = signature.trim() + ')';

					result.push(createNewProposal(CompletionItemKind.Function, word, { signature: signature }));
				}
			}
		}

		return Promise.resolve(result);
	}
}