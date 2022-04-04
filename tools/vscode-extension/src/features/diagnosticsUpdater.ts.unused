import { TextDocument, Position, Uri, Diagnostic, Range, DiagnosticSeverity, TextEditor, languages, TextDocumentChangeEvent, Location } from 'vscode';
import { callEviLint, eviLintType, eviLintDiagnostics } from './utils/eviLintUtil';

const collection = languages.createDiagnosticCollection('evi');

function eviDiagnosticsCallback(document: TextDocument, retry: boolean = false): void {
	if(!document) { collection.clear(); return; }

	const errors: eviLintDiagnostics = callEviLint(document, eviLintType.getDiagnostics, new Position(0, 0));
	if(!errors) {
		if(!retry) eviDiagnosticsCallback(document, true);
		else { console.log("evi lint failed to get errors."); return; }
	}

	let diagnostics: { [file: string]: Diagnostic[] } = {};
	
	// gather all diagnostics sorted by file
	errors.errors.forEach(error => {
		if(!diagnostics[error.position.file]) diagnostics[error.position.file] = [];

		const start: Position = new Position(error.position.line, error.position.column);
		const end: Position = start.translate(0, error.position.length);

		let diagnostic: Diagnostic = new Diagnostic(new Range(start, end), error.message);
		diagnostic.source = 'evi';
		// diagnostic.code = some code for CodeActions,
		diagnostic.relatedInformation = [];

		// add related information
		error.related.forEach(info => {
			const start: Position = new Position(info.position.line, info.position.column);
			const end: Position = start.translate(0, info.position.length);

			diagnostic.relatedInformation.push({
				location: { uri: Uri.file(info.position.file), range: new Range(start, end) },
				message: info.message,
			});
		});

		// set severity
		if(error.type == "error") diagnostic.severity = DiagnosticSeverity.Error;
		else if(error.type == "warning") diagnostic.severity = DiagnosticSeverity.Warning;
		
		diagnostics[error.position.file].push(diagnostic);
	});

	// set al diagnostics per file
	for (let file in diagnostics) collection.set(Uri.file(file), diagnostics[file]);
}

export const eviDiagnosticsChangeActiveEditorCallback = (editor: TextEditor) => {
	if(editor.document.languageId != 'evi') return;
	collection.delete(editor.document.uri);
	eviDiagnosticsCallback(editor.document);
}

export const eviDiagnosticsChangeTextCallback = (event: TextDocumentChangeEvent) => {
	if(event.document.languageId != 'evi') return;
	collection.delete(event.document.uri);
	eviDiagnosticsCallback(event.document);
}

export const eviDiagnosticsCloseEditorCallback = (document: TextDocument) => {
	if(document.languageId != 'evi') return;
	collection.delete(document.uri);
};