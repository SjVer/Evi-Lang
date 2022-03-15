import * as vscode from 'vscode';

import EviCompletionItemProvider from './features/completionItemProvider';
import EviHoverProvider from './features/hoverProvider';
import EviDefenitionProvider from './features/definitionProvider';
import EviSignatureHelpProvider from './features/signatureHelpProvider';
// import EviValidationProvider from './features/validationProvider';
import { eviDiagnosticsChangeActiveEditorCallback, eviDiagnosticsChangeTextCallback, eviDiagnosticsCloseEditorCallback } from './features/diagnosticsUpdater';

export function activate(context: vscode.ExtensionContext): any {

	// let validator = new EviValidationProvider();
	// validator.activate(context.subscriptions);

	// refresh diagnostics if neccesary
	if (vscode.window.activeTextEditor) eviDiagnosticsChangeActiveEditorCallback(vscode.window.activeTextEditor);
	
	// add providers
	context.subscriptions.push(vscode.languages.registerCompletionItemProvider('evi', new EviCompletionItemProvider()));
	context.subscriptions.push(vscode.languages.registerHoverProvider('evi', new EviHoverProvider()));
	context.subscriptions.push(vscode.languages.registerDefinitionProvider('evi', new EviDefenitionProvider()));
	context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(eviDiagnosticsChangeActiveEditorCallback));
	context.subscriptions.push(vscode.workspace.onDidChangeTextDocument(eviDiagnosticsChangeTextCallback));
	context.subscriptions.push(vscode.workspace.onDidCloseTextDocument(eviDiagnosticsCloseEditorCallback));
	context.subscriptions.push(vscode.languages.registerSignatureHelpProvider('evi', new EviSignatureHelpProvider(), '(', ','));
}