import { TextDocument, Position, window, workspace, MarkdownString, Uri } from 'vscode';
import { BackwardIterator } from './backwardIterator';
import { execSync } from 'child_process';
import { copyFile, rm, writeFileSync } from 'fs';
import { basename, dirname, join } from 'path';
import { tmpdir } from 'os';

export enum eviLintType {
	getDeclaration = 'declaration',
	getErrors = 'errors',
	getFunctions = 'functions',
	getVariables = 'variables',
}

export interface eviLintPosition { file: string, line: number, column: number, length: number };
export interface eviLintDeclaration { position: eviLintPosition };
export interface eviLintErrors { errors: { position: eviLintPosition, message: string, related: { position: eviLintPosition, message: string }[] }[] };
export interface eviLintFunctions { functions: { identifier: string, return_type: string, parameters: string[], variadic: boolean }[] };
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

	// remove ansii escape codes
	output = output.replace(RegExp(String.fromCharCode(0x1b) + "\\[([0-9]+;)?[0-9]+m", "g"), '');

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
							length: info['length'] > 0 ? info['length'] : document.lineAt(info['line'] - 1).range.end.character,
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
					parameters: data[func]['parameters'],
					variadic: data[func]['variadic'] 
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



export interface FuncDocumentation { main: string, params: string[], ret?: string };

export async function getDocumentation(document: TextDocument, position: Position): Promise<FuncDocumentation> {
	// get defenition location of function
	const declaration: eviLintDeclaration = callEviLint(document, eviLintType.getDeclaration, position);
	if (!declaration) return { main: "Function declaration not found.", params: [] };
	
	let delcdoc: TextDocument = await workspace.openTextDocument(Uri.file(declaration.position.file));
	if (!delcdoc) return { main: "Could not open file " + declaration.position.file, params: [] };

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
	// for (const param in params) doc += `\n*@param* \`${param}\` - ${params[param]}`;
	
	// get return type
	let ret: string = undefined;
	const retRegex = /\n\s*\@ret\s+(.*)\n/;
	if (retRegex.test(doc)) {
		const match = doc.match(retRegex);
		doc = doc.replace(match[0], "\n");
		// doc += `\n*@ret* - ${match[1]}`;
		ret = match[1];
	}

	return { main: doc, params: params, ret: ret };
	// // format final documentation
	// return new MarkdownString(doc.trim().replaceAll("\n", " \\\n"));
}

export async function getDocumentationAsString(document: TextDocument, position: Position): Promise<MarkdownString> {
	const doc: FuncDocumentation = await getDocumentation(document, position);

	let text = doc.main
	for (let i = 0; i < doc.params.length; i++)
		text += `\n*@param* \`${i}\` - ${doc.params[i]}`;
	if (doc.ret)
		text += `\n*@ret* - ${doc.ret}`;
	
	return new MarkdownString(text.trim().replaceAll("\n", " \\\n"));
}