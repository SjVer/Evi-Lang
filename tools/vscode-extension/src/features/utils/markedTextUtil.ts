import { MarkdownString, MarkedString } from 'vscode';

export function textToMarkedString(text: string): MarkdownString {
	return new MarkdownString(text.replace(/[\\`*_{}[\]()#+\-.!]/g, '\\$&'));
	// return text.replace(/[\\`*_{}[\]()#+\-.!]/g, '\\$&'); // escape markdown syntax tokens: http://daringfireball.net/projects/markdown/syntax#backslash
}