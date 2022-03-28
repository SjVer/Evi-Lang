import { TaskDefinition, TaskProvider, Task, TaskScope, Pseudoterminal, CustomExecution, EventEmitter, Event, FileSystemWatcher, TerminalDimensions, workspace } from 'vscode';
import { exec } from 'child_process';

interface EviTaskDefinition extends TaskDefinition { flags?: string[] }

export class EviTaskProvider implements TaskProvider {
	static eviTaskType: string = "evi";
	private tasks: Task[] | undefined;
	private sharedState: string | undefined;

	constructor(private workspaceRoot: string) { }
	public async provideTasks(): Promise<Task[]> { return this.getTasks(); }

	public resolveTask(task: Task): Task | undefined {
		const definition: EviTaskDefinition = <any>task.definition;
		return this.getTask(definition.flags ? definition.flags : [], definition);
	}

	private getTasks(): Task[] {
		if (this.tasks !== undefined) return this.tasks;
		else return [ this.getTask([]) ];
	}

	private getTask(flags: string[], definition?: EviTaskDefinition): Task {
		if (definition === undefined) definition = { type: EviTaskProvider.eviTaskType, flags };

		return new Task(definition, TaskScope.Workspace, `${flags.join(' ')}`,
			EviTaskProvider.eviTaskType, new CustomExecution(async (): Promise<Pseudoterminal> => {
				// When the task is executed, this callback will run. Here, we setup for running the task.
				return new EviTaskTerminal(this.workspaceRoot, flags, () => this.sharedState, (state: string) => this.sharedState = state);
			}));
	}
}

class EviTaskTerminal implements Pseudoterminal {
	private writeEmitter = new EventEmitter<string>();
	onDidWrite: Event<string> = this.writeEmitter.event;
	private closeEmitter = new EventEmitter<number>();
	onDidClose?: Event<number> = this.closeEmitter.event;

	private eviExecutable: string = workspace.getConfiguration('evi').get<string>('eviExecutablePath'); 

	private fileWatcher: FileSystemWatcher | undefined;

	constructor(private workspaceRoot: string, private flags: string[], private getSharedState: () => string | undefined, private setSharedState: (state: string) => void) {
	}

	open(_initialDimensions: TerminalDimensions | undefined): void {
		// At this point we can start using the terminal.

		// if (this.flags.indexOf('watch') > -1) {
		// 	const pattern = path.join(this.workspaceRoot, 'customBuildFile');
		// 	this.fileWatcher = workspace.createFileSystemWatcher(pattern);
		// 	this.fileWatcher.onDidChange(() => this.doBuild());
		// 	this.fileWatcher.onDidCreate(() => this.doBuild());
		// 	this.fileWatcher.onDidDelete(() => this.doBuild());
		// }

		this.doBuild();
	}

	close(): void {
		// The terminal has been closed. Shutdown the build.
		if (this.fileWatcher) {
			this.fileWatcher.dispose();
		}
	}

	private async doBuild(): Promise<void> {
		let result: { error, stdout: string, sterr: string};

		const cmd = `${this.eviExecutable} ~/Coding/Languages/Evi-Lang/test/test.evi ${this.flags.join(' ')}`;
		exec(cmd, (error, stdout, stderr) => {
			result.error = error;
			result.stdout = stdout;
			result.sterr = stderr;
		});

		this.writeEmitter.fire(result.stdout);

		this.closeEmitter.fire(0);
	}
}