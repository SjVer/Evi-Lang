import { CancellationToken, Definition, DefinitionProvider, LocationLink, Position, ProviderResult, TextDocument, Range, window, Uri } from 'vscode';
import { callEviLint, eviLintDeclaration, eviLintType } from './utils/eviLintUtil';

export default class EviDefenitionProvider implements DefinitionProvider {

	public provideDefinition(document: TextDocument, position: Position, token: CancellationToken): ProviderResult<Definition | LocationLink[]> {
		let result: eviLintDeclaration;
		callEviLint(document, eviLintType.getDeclaration, position)
			.then(_result => result = _result)
			.catch(_reason => { return Promise.reject("Declaration not found."); });
		
		const start: Position = new Position(result.position.line, result.position.column);
		const end: Position = start.translate(0, result.position.length);
		return { uri: Uri.file(result.position.file), range: new Range(start, end) };
	}
}