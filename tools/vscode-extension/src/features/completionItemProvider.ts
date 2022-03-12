import { execSync } from 'child_process';
import { CompletionItemProvider, CompletionItem, CompletionItemKind, CancellationToken, TextDocument, 
		Position, Range, workspace, CompletionContext, window } from 'vscode';
import eviSymbols = require('./eviSymbols');

const identifierRegex: RegExp = /([A-z_]+[a-zA-Z0-9_]*)/g;

enum eviLintType {
	getParameters = 'get-parameters',
	getVariables = 'get-variables',
	getFunctions = 'get-functions',
}

function callEviLint(document: TextDocument, type: eviLintType, position: Position) {
	const file = workspace.getConfiguration('evi').get<string>('eviExecutablePath');
	const cmd = `${file} ${document.fileName} --lint-type="${type}" --lint-pos="${position.line + 1}:${position.character}"`

	window.showInformationMessage(cmd);

	let output: string;
	try { output = execSync(cmd).toString(); }
	catch (error) { window.showErrorMessage(`Failed to execute Evi binary:\n\"${error}\"`); }

	let data: any;
	try { data = JSON.parse(output); }
	catch (error) { window.showErrorMessage(`Failed to parse data returned by Evi binary:\n"${error}"\nData: "${output}"`) }

	window.showInformationMessage(data);
}

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
				callEviLint(document, eviLintType.getParameters, position);
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
			let text = document.getText(new Range(new Position(0, 0), range.start));
			let declMatch = /@\s*([A-z_]+[a-zA-Z0-9_]*)/g;
			let callMatch = /([A-z_]+[a-zA-Z0-9_]*)\s*\(/g;

			// check variables
			let match: RegExpExecArray | null = null;
			while (match = declMatch.exec(text) || callMatch.exec(text)) {
				let word = match[1];
				if (!added[word]) {
					added[word] = true;
					result.push(createNewProposal(CompletionItemKind.Function, word, null));
				}
			}
		}

		return Promise.resolve(result);
	}
}