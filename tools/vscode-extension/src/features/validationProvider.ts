import { isAbsolute } from 'path';
import * as vscode from 'vscode';
import * as which from 'which';
import { ThrottledDelayer } from './utils/async';
import { callEviLint, eviLintType, eviLintDiagnostics } from './utils/eviLintUtil';

export default class EviValidationProvider {

	private validationEnabled: boolean;
	private pauseValidation: boolean;
	private config: IPhpConfig | undefined;
	private loadConfigP: Promise<void>;

	private documentListener: vscode.Disposable | null = null;
	private diagnosticCollection?: vscode.DiagnosticCollection;
	private delayers?: { [key: string]: ThrottledDelayer<void> };

	constructor() {
		this.validationEnabled = true;
		this.pauseValidation = false;
		this.loadConfigP = this.loadConfiguration();
	}

	public activate(subscriptions: vscode.Disposable[]) {
		this.pauseValidation = false;
		this.diagnosticCollection = vscode.languages.createDiagnosticCollection();
		subscriptions.push(this);
		subscriptions.push(vscode.workspace.onDidChangeConfiguration(() => this.loadConfigP = this.loadConfiguration()));

		vscode.workspace.onDidOpenTextDocument(this.triggerValidate, this, subscriptions);
		vscode.workspace.onDidCloseTextDocument((textDocument) => {
			this.diagnosticCollection!.delete(textDocument.uri);
			if (this.delayers) {
				delete this.delayers[textDocument.uri.toString()];
			}
		}, null, subscriptions);
	}

	public dispose(): void {
		if (this.diagnosticCollection) {
			this.diagnosticCollection.clear();
			this.diagnosticCollection.dispose();
		}
		if (this.documentListener) {
			this.documentListener.dispose();
			this.documentListener = null;
		}
	}

	private async loadConfiguration(): Promise<void> {
		// const section = vscode.workspace.getConfiguration();
		const oldExecutable = this.config?.executable;
		// this.validationEnabled = section.get<boolean>(Setting.Enable, true);
		this.validationEnabled = true;

		this.config = await getConfig();

		this.delayers = Object.create(null);
		if (this.pauseValidation) {
			this.pauseValidation = oldExecutable === this.config.executable;
		}
		if (this.documentListener) {
			this.documentListener.dispose();
			this.documentListener = null;
		}
		this.diagnosticCollection!.clear();
		if (this.validationEnabled) {
			this.documentListener = vscode.workspace.onDidChangeTextDocument((e) => { this.triggerValidate(e.document); });
			// Configuration has changed. Reevaluate all documents.
			vscode.workspace.textDocuments.forEach(this.triggerValidate, this);
		}
	}

	private async triggerValidate(textDocument: vscode.TextDocument): Promise<void> {
		await this.loadConfigP;
		if (textDocument.languageId !== 'evi' || this.pauseValidation || !this.validationEnabled) return;
		
		let key = textDocument.uri.toString();
		let delayer = this.delayers![key];
		if (!delayer) {
			// delayer = new ThrottledDelayer<void>(this.config?.trigger === RunTrigger.onType ? 250 : 0);
			delayer = new ThrottledDelayer<void>(250);
			this.delayers![key] = delayer;
		}
		
		delayer.trigger(() => this.doValidate(textDocument));
	}

	private doValidate(textDocument: vscode.TextDocument): Promise<void> {
		return new Promise<void>(resolve => {
			const executable = this.config!.executable;
			if (!executable) {
				this.showErrorMessage('Cannot validate since a Evi installation could not be found. Use the setting \'evi.eviExecutablePath\' to configure the Evi executable.');
				this.pauseValidation = true;
				resolve();
				return;
			}

			if (!isAbsolute(executable)) {
				// executable should either be resolved to an absolute path or undefined.
				// This is just to be sure.
				return;
			}

			let diagnostics: { [file: string]: vscode.Diagnostic[] } = {};
			callEviLint(textDocument, eviLintType.getDiagnostics).then((result: eviLintDiagnostics) => {
				// success
				this.diagnosticCollection.clear();
				
				result.diagnostics.forEach(diagnostic => {
					if(!diagnostics[diagnostic.position.file]) diagnostics[diagnostic.position.file] = [];

					const start: vscode.Position = new vscode.Position(diagnostic.position.line, diagnostic.position.column);
					const end: vscode.Position = start.translate(0, diagnostic.position.length);

					let vsdiagnostic: vscode.Diagnostic = new vscode.Diagnostic(new vscode.Range(start, end), diagnostic.message);
					vsdiagnostic.source = 'evi';
					// vsdiagnostic.code = some code for CodeActions,
					vsdiagnostic.relatedInformation = [];

					// add related information
					diagnostic.related.forEach(info => {
						const start: vscode.Position = new vscode.Position(info.position.line, info.position.column);
						const end: vscode.Position = start.translate(0, info.position.length);

						vsdiagnostic.relatedInformation.push({
							location: { uri: vscode.Uri.file(info.position.file), range: new vscode.Range(start, end) },
							message: info.message,
						});
					});

					// set severity
					if(diagnostic.type == "error") vsdiagnostic.severity = vscode.DiagnosticSeverity.Error;
					else if(diagnostic.type == "warning") vsdiagnostic.severity = vscode.DiagnosticSeverity.Warning;
					
					diagnostics[diagnostic.position.file].push(vsdiagnostic);
				});

				for (let file in diagnostics) this.diagnosticCollection.set(vscode.Uri.file(file), diagnostics[file]);
				resolve();

			}).catch((error) => {
				// failure
				console.warn(`eviLint failure in doValidate: ${error}`);
				this.pauseValidation = true;
				resolve();
			});
		});
	}

	private async showError(error: any, executable: string): Promise<void> {
		let message: string = error.message ? error.message : `Failed to run evi using path: ${executable}. Reason is unknown.`;
		if (!message) return;

		return this.showErrorMessage(message);
	}

	private async showErrorMessage(message: string): Promise<void> {
		const openSettings = 'Open Settings';
		if (await vscode.window.showInformationMessage(message, openSettings) === openSettings) {
			vscode.commands.executeCommand('workbench.action.openSettings', 'evi.eviExecutablePath');
		}
	}
}

interface IPhpConfig {
	readonly executable: string | undefined;
	readonly executableIsUserDefined: boolean | undefined;
	// readonly trigger: RunTrigger;
}

async function getConfig(): Promise<IPhpConfig> {
	const section = vscode.workspace.getConfiguration();

	let executable: string | undefined;
	let executableIsUserDefined: boolean | undefined;
	const inspect = section.inspect<string>('evi.eviExecutablePath');
	if (inspect && inspect.workspaceValue) {
		executable = inspect.workspaceValue;
		executableIsUserDefined = false;
	} else if (inspect && inspect.globalValue) {
		executable = inspect.globalValue;
		executableIsUserDefined = true;
	} else {
		executable = undefined;
		executableIsUserDefined = undefined;
	}

	if (executable && !isAbsolute(executable)) {
		const first = vscode.workspace.workspaceFolders && vscode.workspace.workspaceFolders[0];
		if (first) {
			executable = vscode.Uri.joinPath(first.uri, executable).fsPath;
		} else {
			executable = undefined;
		}
	} else if (!executable) {
		executable = await getEviPath();
	}

	// const trigger = RunTrigger.from(section.get<string>(Setting.Run, RunTrigger.strings.onSave));
	// const trigger = RunTrigger.onType;
	return {
		executable,
		executableIsUserDefined,
		// trigger
	};
}

async function getEviPath(): Promise<string | undefined> {
	try { return await which('evi');
	} catch (e) { return undefined; }
}