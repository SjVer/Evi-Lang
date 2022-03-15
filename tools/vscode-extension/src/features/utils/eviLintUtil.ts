import { TextDocument, Position, window, workspace } from 'vscode';
import { execSync } from 'child_process';
import { copyFile, rm, writeFileSync } from 'fs';
import { basename, dirname, join } from 'path';
import { tmpdir } from 'os';
import { stringify } from 'querystring';

export enum eviLintType {
	getDeclaration = 'declaration',
	getErrors = 'errors',
	getFunctions = 'functions',
	getVariables = 'variables',
}

export interface eviLintPosition { file: string, line: number, column: number, length: number };
export interface eviLintDeclaration { position: eviLintPosition };
export interface eviLintErrors { errors: { position: eviLintPosition, message: string, related: { position: eviLintPosition, message: string }[] }[] };
export interface eviLintFunctions { functions: { identifier: string, return_type: string, parameters: string[] }[] };
export interface eviLintVariables { variables: { identifier: string, type: string }[] };

let do_log: boolean = workspace.getConfiguration('evi').get<boolean>('logDebugInfo');
function log(message?: any, ...optionalParams: any[]): void {
	if (do_log) console.log(message, ...optionalParams);
}

export function callEviLint(document: TextDocument, type: eviLintType, position: Position): any {
	// copy file so that unsaved changes are included
	let tmpfile = join(tmpdir(), basename(document.fileName) + '.evilint_tmp');
	copyFile(document.fileName, tmpfile, (err) => {
		if(!err) return;
		window.showErrorMessage(`Evi lint failed to copy file "${document.fileName}".`);
		tmpfile = document.fileName;
	});
	writeFileSync(tmpfile, document.getText());
	
	const dir = dirname(document.fileName);
	const eviExec = workspace.getConfiguration('evi').get<string>('eviExecutablePath');
	const cmd = `${eviExec} ${tmpfile} ` +
				`--include="${dir}" ` +
				`--lint-type="${type}" ` +
				`--lint-pos="${position.line + 1}:${position.character}" ` +
				`--lint-tab-width="${workspace.getConfiguration('editor').get<number>('tabSize')}" `;

	if (tmpfile.endsWith('.evilint_tmp')) rm(tmpfile, () => {});

	log("\nlint cmd: " + cmd);

	let output: string;
	try { output = execSync(cmd).toString(); }
	catch (error) { window.showErrorMessage(`Failed to execute Evi binary:\n\"${error}\"`); }

	log("lint output: " + output);
	
	let data: any;
	try { data = JSON.parse(output); }
	catch (error) { window.showErrorMessage(`Failed to parse data returned by Evi binary:\n"${error}"\nData: "${output}"`); return undefined }

	try { switch (type)
	{
		case eviLintType.getDeclaration: {
			if (data['invalid']) return undefined;

			let result: eviLintDeclaration = {
				position: {
					file: data['file'] == tmpfile ? document.fileName : data['file'],
					line: data['line'] - 1,
					column: data['column'],
					length: data['length'],
				},
			};

			log(result as any);
			return result;
		}
		case eviLintType.getErrors: {
			let result: eviLintErrors = { errors: [] };
			data.forEach((error: any) => {
				// gather related information
				let related: { position: eviLintPosition, message: string }[] = [];
				
				error['related'].forEach((info) => {
					related.push({
						position: {
							file: info['file'] == tmpfile ? document.fileName : info['file'],
							line: info['line'] - 1,
							column: info['column'],
							length: info['length'],
						},
						message: info['message'],
					});
				});

				// create error itself
				result.errors.push({
					position: {
						file: error['file'] == tmpfile ? document.fileName : error['file'],
						line: error['line'] - 1,
						column: error['column'],
						length: error['length'],
					},
					message: error['message'],
					related: related,
				});
			});
			log(result as any);
			return result;
		}
		case eviLintType.getFunctions: {
			let result: eviLintFunctions = { functions: [] };
			for (let func in data) {
				result.functions.push({
					identifier: func,
					return_type: data[func]['return type'],
					parameters: data[func]['parameters']
				});
			}
			log(result as any);
			return result;
		}
		case eviLintType.getVariables: {
			let result: eviLintVariables = { variables: [] };
			for (let varr in data) {
				result.variables.push({
					identifier: varr,
					type: data[varr]
				});
			}
			log(result as any);
			return result;
		}
	} } catch (e) { log(e); }
}