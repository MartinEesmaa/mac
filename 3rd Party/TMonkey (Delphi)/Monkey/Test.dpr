program Test;

uses
  Forms,
  Main in 'Main.pas' {MainForm},
  Monkey in 'Monkey.pas',
  ID3v1 in '..\ID3v1\ID3v1.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.Title := 'Monkey Test';
  Application.CreateForm(TMainForm, MainForm);
  Application.Run;
end.
