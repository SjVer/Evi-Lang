import { TextDocument, Position, window, workspace } from 'vscode';
import { execSync } from 'child_process';
import { copyFile, rm, writeFileSync } from 'fs';
import { basename, dirname, join } from 'path';
import { tmpdir } from 'os';

export enum eviLintType {
	getFunctions = 'functions',
	getVariables = 'variables',
}

export interface eviLintResult { elements: { identifier: string, properties: any }[] };

export function callEviLint(document: TextDocument, type: eviLintType, position: Position): eviLintResult {

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
	const cmd = `${eviExec} ${tmpfile} --include="${dir}" --lint-type="${type}" --lint-pos="${position.line + 1}:${position.character}"`

	if (tmpfile.endsWith('.evilint_tmp')) rm(tmpfile, () => {});

	let output: string;
	try { output = execSync(cmd).toString(); }
	catch (error) { window.showErrorMessage(`Failed to execute Evi binary:\n\"${error}\"`); }

	let data: eviLintResult = { elements: [] };
	let callback;
	
	switch (type) {
		case eviLintType.getVariables:
		{
			callback = (key: string, value: string) => {
				if(!key) return;
				data.elements.push({
					identifier: key,
					properties: value
				});
			}
			break;
		}
		case eviLintType.getFunctions:
		{
			callback = (key: string, value: string) => {
				if(!key) return;
				data.elements.push({
					identifier: key,
					properties: value
				});
			}
			break;
		}
	
		default:
			callback = () => {};
			break;
	}

	try { JSON.parse(output, callback); }
	catch (error) { window.showErrorMessage(`Failed to parse data returned by Evi binary:\n"${error}"\nData: "${output}"`) }

	return data;
}