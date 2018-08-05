type state = {editor: ref(option(CodeMirror.editor))};

let component = ReasonReact.reducerComponent("Editor_CodeBlock");

let getEditor = (state, ~default, ~f) =>
  switch (state.editor^) {
  | None => default
  | Some(editor) => f(editor)
  };

let make =
    (
      ~value,
      ~focused,
      ~onChange,
      ~onFocus=?,
      ~onBlur=?,
      ~onBlockUp=?,
      ~onBlockDown=?,
      _children,
    )
    : ReasonReact.component(state, _, unit) => {
  ...component,
  initialState: () => {editor: ref(None)},
  render: ({state}) =>
    <Editor_CodeMirror
      value
      focused
      onChange
      ?onFocus
      ?onBlur
      ?onBlockUp
      ?onBlockDown
      setEditor=(
        editor => {
          state.editor := Some(editor);
          ();
        }
      )
      options=(
        CodeMirror.EditorConfiguration.make(
          ~mode="gfm",
          ~theme="rtop-light",
          ~placeholder="Add your thoughts here",
          ~lineNumbers=false,
          ~viewportMargin=infinity,
          ~lineWrapping=true,
          (),
        )
      )
    />,
};
