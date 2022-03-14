import { TextDocument, Position, window, workspace } from 'vscode';
import { execSync } from 'child_process';
import { copyFile, rm, writeFileSync } from 'fs';
import { basename, dirname, join } from 'path';
import { tmpdir } from 'os';
import { stringify } from 'querystring';

export enum eviLintType {
	getFunctions = 'functions',
	getVariables = 'variables',
}

export interface eviLintFunctions { elements: { identifier: string, return_type: string, parameters: string[] }[] };
export interface eviLintVariables { elements: { identifier: string, type: string }[] };

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

	console.log("lint cmd: " + cmd);

	let output: string;
	try { output = execSync(cmd).toString(); }
	catch (error) { window.showErrorMessage(`Failed to execute Evi binary:\n\"${error}\"`); }

	console.log("lint output: " + output);

	let data;
	try { data = JSON.parse(output); }
	catch (error) { window.showErrorMessage(`Failed to parse data returned by Evi binary:\n"${error}"\nData: "${output}"`) }

	try { switch (type)
	{
		case eviLintType.getFunctions: {
			let result: eviLintFunctions = { elements: [] };
			for (let func in data) {
				result.elements.push({
					identifier: func,
					return_type: data[func]['~'],
					parameters: []
				})
				for (let param in data[func])
					if(param != '~') result.elements[result.elements.length - 1]
						.parameters.push(data[func][param]);
			}
			return result;
		}
		case eviLintType.getVariables: {
			console.log(data);
			return { elements: [] } as eviLintVariables;
		}
	} } catch (e) { console.log(e); }
}