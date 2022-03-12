import * as vscode from 'vscode';

import EviCompletionItemProvider from './features/completionItemProvider';
import EviHoverProvider from './features/hoverProvider';
// import EviSignatureHelpProvider from './features/signatureHelpProvider';
// import EviValidationProvider from './features/validationProvider';

export function activate(context: vscode.ExtensionContext): any {

	// let validator = new EviValidationProvider();
	// validator.activate(context.subscriptions);

	// add providers
	context.subscriptions.push(vscode.languages.registerCompletionItemProvider('evi', new EviCompletionItemProvider()));
	context.subscriptions.push(vscode.languages.registerHoverProvider('evi', new EviHoverProvider()));
	// context.subscriptions.push(vscode.languages.registerSignatureHelpProvider('Evi', new EviSignatureHelpProvider(), '(', ','));
}