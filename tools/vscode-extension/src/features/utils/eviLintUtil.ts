import { TextDocument, Position, window, workspace, MarkdownString, Uri } from 'vscode';
import { BackwardIterator } from './backwardIterator';
import { execSync } from 'child_process';
import { copyFileSync, rmSync, writeFileSync } from 'fs';
import { basename, dirname, join } from 'path';
import { tmpdir } from 'os';
import { waitUntil } from './async';

export enum eviLintType {
	getDeclaration = 'get-declaration',
	getDiagnostics = 'get-diagnostics',
	getFunctions = 'get-functions',
	getVariables = 'get-variables',
}

export interface eviLintPosition { file: string, line: number, column: number, length: number };
export interface eviLintDeclaration { position: eviLintPosition };
export interface eviLintDiagnostics { diagnostics: { position: eviLintPosition, message: string, type: string, related: { position: eviLintPosition, message: string }[] }[] };
export interface eviLintFunctions { functions: { identifier: string, return_type: string, parameters: string[], variadic: boolean }[] };
export interface eviLintVariables { variables: { identifier: string, type: string }[] };

let do_log: boolean = workspace.getConfiguration('evi').get<boolean>('logDebugInfo');
function log(message?: any, ...optionalParams: any[]): void {
	if (do_log) console.log(message, ...optionalParams);
}

let blocked = false;

export async function callEviLint(document: TextDocument, type: eviLintType, position?: Position): Promise<any> {
	if(blocked) waitUntil(() => !blocked, 3000);
	blocked = true;

	log(`\n\n\n====== LINT: ${type.toUpperCase()} ======`);

	// copy file so that unsaved changes are included
	let tmpfile = join(tmpdir(), basename(document.fileName) + '.evilint_tmp');
	copyFileSync(document.fileName, tmpfile);
	writeFileSync(tmpfile, document.getText());
	
	const dir = dirname(document.fileName);
	const eviExec = workspace.getConfiguration('evi').get<string>('eviExecutablePath');
	const workspacefolder = workspace.getWorkspaceFolder(document.uri).uri.fsPath;

	let pos = position ? `${position.line + 1}:${position.character}` : "0:0";
	let cmd = `${eviExec} ${tmpfile} ` +
				`--include="${dir}" ` +
				`--lint-type="${type}" ` +
				`--lint-pos="${pos}" ` +
				`--lint-tab-width="${workspace.getConfiguration('editor').get<number>('tabSize')}" `;
	workspace.getConfiguration('evi').get<string[]>('includeSearchPaths').forEach(path =>
		cmd += ` --include=${path}`.replace('${workspaceFolder}', workspacefolder));

	log("lint cmd: " + cmd);
	
	let output: string;
	try { output = execSync(cmd).toString(); }
	catch (error) { console.warn(`Failed to execute Evi binary:\n\"${error}\"`); }
	if (tmpfile.endsWith('.evilint_tmp')) rmSync(tmpfile);
	
	blocked = false;

	// remove ansii escape codes
	output = output.replace(RegExp(String.fromCharCode(0x1b) + "\\[([0-9]+;)?[0-9]+m", "g"), '');
	log("lint output: " + output);
	
	let data: any;
	try { data = JSON.parse(output); }
	catch (error) { console.warn(`Failed to parse data returned by Evi binary:\n"${error}"\nData: "${output}"`); return Promise.reject() }

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
			return Promise.resolve(result);
		}
		case eviLintType.getDiagnostics: {
			let result: eviLintDiagnostics = { diagnostics: [] };
			data.forEach((error: any) => {
				if(error['invalid']) return;

				// gather related information
				let related: { position: eviLintPosition, message: string }[] = [];
				
				error['related'].forEach((info) => {
					related.push({
						position: {
							file: info['file'] == tmpfile ? document.fileName : info['file'],
							line: info['line'] - 1,
							column: info['column'],
							length: info['length'] > 0 ? info['length'] : document.lineAt(info['line'] - 1).range.end.character,
						},
						message: info['message'],
					});
				});

				// create error itself
				result.diagnostics.push({
					position: {
						file: error['file'] == tmpfile ? document.fileName : error['file'],
						line: error['line'] - 1,
						column: error['column'],
						length: error['length'],
					},
					message: error['message'],
					type: error['type'],
					related: related,
				});
			});
			log(result as any);
			return Promise.resolve(result);
		}
		case eviLintType.getFunctions: {
			let result: eviLintFunctions = { functions: [] };
			for (let func in data) {
				result.functions.push({
					identifier: func,
					return_type: data[func]['return type'],
					parameters: data[func]['parameters'],
					variadic: data[func]['variadic'] 
				});
			}
			log(result as any);
			return Promise.resolve(result);
		}
		case eviLintType.getVariables: {
			let result: eviLintVariables = { variables: [] };
			for (let varr in data) {
				result.variables.push({
					identifier: varr,
					type: data[varr]
				});
			}
			return Promise.resolve(result);
		}
	} } catch (e) { log(e); }
}

export interface FuncDocumentation { main: string, params: string[], ret?: string };

export async function getDocumentation(document: TextDocument, position: Position): Promise<FuncDocumentation> {
	// get defenition location of function
	const declaration: eviLintDeclaration = await callEviLint(document, eviLintType.getDeclaration, position);
	if (!declaration) return Promise.reject("Function declaration not found.");

	let delcdoc: TextDocument = await workspace.openTextDocument(Uri.file(declaration.position.file));
	if (!delcdoc) return Promise.reject("Could not open file " + declaration.position.file);

	let it = new BackwardIterator(delcdoc, declaration.position.column, declaration.position.line);

	// get newline before declaration
	while (it.hasNext()) { if (it.next() == "\n") break; };

	// get full documentation
	let doc: string = "";
	while (it.hasNext()) {
		const c = it.next();
		if (c == "\n" && !doc.startsWith("\\?")) {
			// end of documentation, remove last (non-doc) line
			doc = doc.slice(doc.indexOf("\\?"));
			
			// test if there's actually a documentation
			if(!doc.startsWith('\\?')) doc = "";
			
			break;
		}
		doc = c + doc;
	}
	// replace comment tokens with just newlines
	doc = doc.replace(/\n?\\\?\s*/g, "\n") + "\n";
	while (doc.startsWith("\n")) doc = doc.slice(1);


	// get parameters
	let params: string[] = [];
	const paramRegex = /\n\s*\@param\s+([0-9]+)\s+(.*)\n/;
	while (paramRegex.test(doc)) {
		const match = doc.match(paramRegex);
		doc = doc.replace(match[0], "\n");
		params.push(match[2]);
	}
	
	// get return type
	let ret: string = undefined;
	const retRegex = /\n\s*\@return\s+(.*)\n/;
	if (retRegex.test(doc)) {
		const match = doc.match(retRegex);
		doc = doc.replace(match[0], "\n");
		// doc += `\n*@return* - ${match[1]}`;
		ret = match[1];
	}

	return Promise.resolve({ main: doc, params: params, ret: ret });
}

export async function getDocumentationAsString(document: TextDocument, position: Position): Promise<MarkdownString> {
	const doc: FuncDocumentation = await getDocumentation(document, position);

	let text = doc.main
	for (let i = 0; i < doc.params.length; i++)
		text += `\n*@param* \`${i}\` - ${doc.params[i]}`;
	if (doc.ret)
		text += `\n*@return* - ${doc.ret}`;
	
	return new MarkdownString(text.trim().replaceAll("\n", " \\\n"));
}