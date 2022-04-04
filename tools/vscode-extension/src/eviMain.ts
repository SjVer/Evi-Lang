import * as vscode from 'vscode';
import { execSync } from 'child_process';

import EviCompletionItemProvider from './features/completionItemProvider';
import EviHoverProvider from './features/hoverProvider';
import EviDefenitionProvider from './features/definitionProvider';
import EviSignatureHelpProvider from './features/signatureHelpProvider';
import EviValidationProvider from './features/validationProvider';
// import { EviTaskProvider } from './features/taskProvider';

export function activate(context: vscode.ExtensionContext): any {
	if(!vscode.workspace.getConfiguration('evi').get<boolean>('enableLanguageFeatures')) return;

	// // get workspace root
	// const workspaceRoot = (vscode.workspace.workspaceFolders && (vscode.workspace.workspaceFolders.length > 0))
	// 					   ? vscode.workspace.workspaceFolders[0].uri.fsPath : undefined;
	// if (!workspaceRoot) return;

	let validator = new EviValidationProvider();
	validator.activate(context.subscriptions);

	// set status bar
	let statusbarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 0);
	try {
		const output = execSync(vscode.workspace.getConfiguration('evi').get<string>('eviExecutablePath') + ' --version').toString();
		statusbarItem.text = output.replace('evi ', 'Evi: ');
		statusbarItem.show();
	}
	catch (error) { vscode.window.showErrorMessage(`Failed to get Evi version:\n\"${error}\"`); }

	// refresh diagnostics if neccesary
	// if (vscode.window.activeTextEditor) diagnostics.eviDiagnosticsChangeActiveEditorCallback(vscode.window.activeTextEditor);
	
	// add providers
	context.subscriptions.push(vscode.languages.registerCompletionItemProvider('evi', new EviCompletionItemProvider()));
	context.subscriptions.push(vscode.languages.registerHoverProvider('evi', new EviHoverProvider()));
	context.subscriptions.push(vscode.languages.registerDefinitionProvider('evi', new EviDefenitionProvider()));
	context.subscriptions.push(vscode.languages.registerSignatureHelpProvider('evi', new EviSignatureHelpProvider(), '(', ','));
	// context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(diagnostics.eviDiagnosticsChangeActiveEditorCallback));
	// context.subscriptions.push(vscode.workspace.onDidChangeTextDocument(diagnostics.eviDiagnosticsChangeTextCallback));
	// context.subscriptions.push(vscode.workspace.onDidCloseTextDocument(diagnostics.eviDiagnosticsCloseEditorCallback));
	// context.subscriptions.push(vscode.tasks.registerTaskProvider(EviTaskProvider.eviTaskType, new EviTaskProvider(workspaceRoot)))
}