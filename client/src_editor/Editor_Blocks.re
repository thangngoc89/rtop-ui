[%%debugger.chrome];
Modules.require("./Editor_Blocks.css");

open Utils;
open Editor_Types;
open Editor_Types.Block;
open Editor_Blocks_Utils;

type action =
  | Block_Add(id, blockTyp)
  | Block_Execute(bool)
  | Block_FocusNextBlockOrCreate
  | Block_QueueDelete(id)
  | Block_DeleteQueued(id)
  | Block_Restore(id)
  | Block_CaptureQueuedMeta(id, Js.Global.timeoutId, blockData)
  | Block_Focus(id, blockTyp)
  | Block_Blur(id)
  | Block_UpdateValue(id, string, CodeMirror.EditorChange.t)
  | Block_AddWidgets(id, array(Widget.t))
  | Block_FocusUp(id)
  | Block_FocusDown(id);

type state = {
  blocks: array(block),
  deletedBlockMeta: array(deletedBlockMeta),
  stateUpdateReason: option(action),
  focusedBlock: option((id, blockTyp, focusChangeType)),
};

let blockControlsButtons = (blockId, isDeleted, send) =>
  <div className="block__controls--buttons">
    <UI_Balloon message="Add code block" position=Down>
      ...<button
           className="block__controls--button"
           ariaLabel="Add code block"
           onClick=(_ => send(Block_Add(blockId, BTyp_Code)))>
           <Fi.Code />
           <sup> "+"->str </sup>
         </button>
    </UI_Balloon>
    <UI_Balloon message="Add text block" position=Down>
      ...<button
           className="block__controls--button"
           ariaLabel="Add text block"
           onClick=(_ => send(Block_Add(blockId, BTyp_Text)))>
           <Fi.Edit2 />
           <sup> "+"->str </sup>
         </button>
    </UI_Balloon>
    (
      !isDeleted ?
        <UI_Balloon message="Delete block" position=Down>
          ...<button
               className="block__controls--button block__controls--danger"
               ariaLabel="Delete block"
               onClick=(_ => send(Block_QueueDelete(blockId)))>
               <Fi.Trash2 />
               <sup> "-"->str </sup>
             </button>
        </UI_Balloon> :
        <UI_Balloon message="Restore block" position=Down>
          ...<button
               className="block__controls--button"
               ariaLabel="Restore block"
               onClick=(_ => send(Block_Restore(blockId)))>
               <Fi.RefreshCw />
               <sup> "+"->str </sup>
             </button>
        </UI_Balloon>
    )
  </div>;

let component = ReasonReact.reducerComponent("Editor_Page");

let make =
    (
      ~lang=RE,
      ~blocks: array(block),
      ~readOnly=false,
      ~onUpdate,
      ~registerExecuteCallback=?,
      ~registerShortcut: option(Shortcut.subscribeFun)=?,
      _children,
    ) => {
  ...component,
  initialState: () => {
    blocks: blocks->syncLineNumber,
    deletedBlockMeta: [||],
    stateUpdateReason: None,
    focusedBlock: None,
  },
  didMount: self => {
    self.send(Block_Execute(false));
    switch (registerExecuteCallback) {
    | None => ()
    | Some(register) => register(() => self.send(Block_Execute(false)))
    };
    switch (registerShortcut) {
    | None => ()
    | Some(registerShortcut) =>
      let unReg =
        registerShortcut(
          ~global=true,
          "mod+enter",
          event => {
            open Webapi.Dom;

            event->KeyboardEvent.preventDefault;
            self.send(Block_Execute(false));
          },
        );
      let unReg2 =
        registerShortcut(
          ~global=true,
          "shift+enter",
          event => {
            open Webapi.Dom;

            event->KeyboardEvent.preventDefault;
            self.send(Block_Execute(true));
          },
        );
      self.onUnmount(() => {
        unReg();
        unReg2();
      });
    };
  },
  didUpdate: ({oldSelf, newSelf}) =>
    if (oldSelf.state.blocks !== newSelf.state.blocks) {
      switch (newSelf.state.stateUpdateReason) {
      | None => ()
      | Some(action) =>
        switch (action) {
        | Block_Focus(_, _)
        | Block_Blur(_)
        | Block_AddWidgets(_, _)
        | Block_FocusUp(_)
        | Block_FocusDown(_)
        | Block_FocusNextBlockOrCreate
        | Block_Execute(_) => ()
        | Block_Add(_, _)
        | Block_QueueDelete(_)
        | Block_DeleteQueued(_)
        | Block_Restore(_)
        | Block_CaptureQueuedMeta(_, _, _)
        | Block_UpdateValue(_, _, _) => onUpdate(newSelf.state.blocks)
        }
      };
    },
  reducer: (action, state) =>
    switch (action) {
    | Block_AddWidgets(blockId, widgets) =>
      ReasonReact.Update({
        ...state,
        stateUpdateReason: Some(action),
        blocks:
          state.blocks
          ->(
              Belt.Array.mapU((. block) => {
                let {b_id, b_data, b_deleted} = block;
                if (b_id != blockId) {
                  block;
                } else {
                  switch (b_data) {
                  | B_Text(_) => block
                  | B_Code(bcode) => {
                      b_id,
                      b_data: B_Code({...bcode, bc_widgets: widgets}),
                      b_deleted,
                    }
                  };
                };
              })
            ),
      })
    | Block_FocusNextBlockOrCreate =>
      let blockLength = state.blocks->Belt.Array.length;
      let nextBlockIndex =
        switch (state.focusedBlock) {
        | None => blockLength - 1
        | Some((id, _blockTyp, _)) =>
          switch (state.blocks->arrayFindIndex((({b_id}) => b_id == id))) {
          | None => blockLength - 1
          | Some(index) => index
          }
        };
      let findBlockId = index => {
        let {b_id, b_data} = state.blocks[index];
        (b_id, blockDataToBlockTyp(b_data));
      };
      if (nextBlockIndex == blockLength - 1) {
        ReasonReact.SideEffects(
          (
            ({send}) =>
              send(Block_Add(findBlockId(nextBlockIndex)->fst, BTyp_Code))
          ),
        );
      } else if (nextBlockIndex < blockLength - 1) {
        let (blockId, blockTyp) = findBlockId(nextBlockIndex + 1);
        ReasonReact.Update({
          ...state,
          stateUpdateReason: Some(action),
          focusedBlock:
            Some((blockId, blockTyp, FcTyp_BlockExecuteAndFocusNextBlock)),
        });
      } else {
        ReasonReact.NoUpdate;
      };
    | Block_Execute(focusNextBlock) =>
      let allCodeBlocks =
        state.blocks
        ->(
            Belt.Array.reduceU([], (. acc, {b_id, b_data}) =>
              switch (b_data) {
              | B_Text(_) => acc
              | B_Code({bc_value}) => [(b_id, bc_value), ...acc]
              }
            )
          )
        ->Belt.List.reverse;

      /* Clear all widgets and execute all blocks */
      ReasonReact.SideEffects(
        (
          self =>
            Js.Promise.(
              Editor_Worker.executeMany(. lang, allCodeBlocks)
              |> then_(results => {
                   results
                   ->(
                       Belt.List.forEachU((. (blockId, result)) => {
                         let widgets = executeResultToWidget(result);
                         self.send(Block_AddWidgets(blockId, widgets));
                       })
                     );

                   resolve();
                 })
              |> then_(() => {
                   if (focusNextBlock) {
                     self.send(Block_FocusNextBlockOrCreate);
                   };
                   resolve();
                 })
              |> catch(error => resolve(Js.log(error)))
              |> ignore
            )
        ),
      );
    | Block_UpdateValue(blockId, newValue, diff) =>
      let blockIndex =
        arrayFindIndex(state.blocks, ({b_id}) => b_id == blockId)
        ->getBlockIndex;

      ReasonReact.Update({
        ...state,
        stateUpdateReason: Some(action),
        blocks:
          state.blocks
          ->(
              Belt.Array.mapWithIndexU((. i, block) => {
                let {b_id, b_data, b_deleted} = block;
                if (i < blockIndex) {
                  block;
                } else if (i == blockIndex) {
                  switch (b_data) {
                  | B_Code(bcode) => {
                      b_id,
                      b_data:
                        B_Code({
                          ...bcode,
                          bc_value: newValue,
                          bc_widgets: {
                            let removeWidgetBelowMe =
                              diff->getFirstLineFromDiff;
                            let currentWidgets = bcode.bc_widgets;
                            currentWidgets
                            ->(
                                Belt.Array.keepU((. {Widget.lw_line, _}) =>
                                  lw_line < removeWidgetBelowMe
                                )
                              );
                          },
                        }),
                      b_deleted,
                    }
                  | B_Text(_) => {b_id, b_data: B_Text(newValue), b_deleted}
                  };
                } else {
                  switch (b_data) {
                  | B_Text(_) => block
                  | B_Code(bcode) => {
                      ...block,
                      b_data: B_Code({...bcode, bc_widgets: [||]}),
                    }
                  };
                };
              })
            )
          ->syncLineNumber,
      });
    | Block_QueueDelete(blockId) =>
      let queueTimeout = (self, b_data) => {
        let timeoutId =
          Js.Global.setTimeout(
            () => self.ReasonReact.send(Block_DeleteQueued(blockId)),
            10000,
          );
        self.ReasonReact.send(
          Block_CaptureQueuedMeta(blockId, timeoutId, b_data),
        );
        ();
      };
      if (isLastBlock(state.blocks)) {
        switch (isEmpty(state.blocks[0].b_data)) {
        | true => ReasonReact.NoUpdate
        | _ =>
          ReasonReact.UpdateWithSideEffects(
            {
              ...state,
              blocks:
                [|newBlock, {...state.blocks[0], b_deleted: true}|]
                ->syncLineNumber,
              stateUpdateReason: Some(action),
              focusedBlock: None,
            },
            (self => queueTimeout(self, state.blocks[0].b_data)),
          )
        };
      } else {
        let blockIndex =
          arrayFindIndex(state.blocks, ({b_id}) => b_id == blockId)
          ->getBlockIndex;
        ReasonReact.UpdateWithSideEffects(
          {
            ...state,
            blocks:
              state.blocks
              ->(
                  Belt.Array.mapWithIndexU((. i, block) =>
                    i == blockIndex ?
                      {...state.blocks[blockIndex], b_deleted: true} : block
                  )
                )
              ->syncLineNumber,
            stateUpdateReason: Some(action),
            focusedBlock:
              switch (state.focusedBlock) {
              | None => None
              | Some((focusedBlock, _, _)) =>
                focusedBlock == blockId ? None : state.focusedBlock
              },
          },
          (self => queueTimeout(self, state.blocks[blockIndex].b_data)),
        );
      };
    | Block_DeleteQueued(blockId) =>
      if (isLastBlock(state.blocks) || Belt.Array.length(state.blocks) == 0) {
        ReasonReact.Update({
          blocks: [|newBlock|],
          deletedBlockMeta:
            state.deletedBlockMeta
            ->(Belt.Array.keepU((. {db_id}) => db_id != blockId)),
          stateUpdateReason: Some(action),
          focusedBlock: None,
        });
      } else {
        ReasonReact.Update({
          blocks:
            state.blocks
            ->(Belt.Array.keepU((. {b_id}) => b_id != blockId))
            ->syncLineNumber,
          deletedBlockMeta:
            state.deletedBlockMeta
            ->(Belt.Array.keepU((. {db_id}) => db_id != blockId)),
          stateUpdateReason: Some(action),
          focusedBlock:
            switch (state.focusedBlock) {
            | None => None
            | Some((focusedBlock, _, _)) =>
              focusedBlock == blockId ? None : state.focusedBlock
            },
        });
      }
    | Block_Restore(blockId) =>
      let blockIndex =
        arrayFindIndex(state.blocks, ({b_id}) => b_id == blockId)
        ->getBlockIndex;

      let restoreMeta =
        state.deletedBlockMeta
        ->Belt.Array.keepU(((. {db_id}) => db_id == blockId));

      let restoredBlock = {
        ...state.blocks[blockIndex],
        b_deleted: false,
        b_data: restoreMeta[0].db_data,
      };

      ReasonReact.UpdateWithSideEffects(
        {
          ...state,
          blocks:
            state.blocks
            ->(
                Belt.Array.mapWithIndexU((. i, block) =>
                  i == blockIndex ? restoredBlock : block
                )
              )
            ->syncLineNumber,
          deletedBlockMeta:
            state.deletedBlockMeta
            ->(Belt.Array.keepU((. {db_id}) => db_id != blockId)),
          stateUpdateReason: Some(action),
        },
        (_self => Js.Global.clearTimeout(restoreMeta[0].to_id)),
      );
    | Block_CaptureQueuedMeta(blockId, timeoutId, data) =>
      let meta = {db_id: blockId, to_id: timeoutId, db_data: data};
      ReasonReact.Update({
        ...state,
        deletedBlockMeta:
          Belt.Array.concat(state.deletedBlockMeta, [|meta|]),
        stateUpdateReason: Some(action),
      });
    | Block_Focus(blockId, blockTyp) =>
      ReasonReact.Update({
        ...state,
        stateUpdateReason: Some(action),
        focusedBlock: Some((blockId, blockTyp, FcTyp_EditorFocus)),
      })
    | Block_Blur(blockId) =>
      switch (state.focusedBlock) {
      | None => ReasonReact.NoUpdate
      | Some((focusedBlockId, _, _)) =>
        focusedBlockId == blockId ?
          ReasonReact.Update({
            ...state,
            stateUpdateReason: Some(action),
            focusedBlock: None,
          }) :
          ReasonReact.NoUpdate
      }
    | Block_Add(afterBlockId, blockTyp) =>
      let newBlockId = generateId();
      ReasonReact.Update({
        ...state,
        stateUpdateReason: Some(action),
        focusedBlock: Some((newBlockId, blockTyp, FcTyp_BlockNew)),
        blocks:
          state.blocks
          ->(
              Belt.Array.reduceU(
                [||],
                (. acc, block) => {
                  let {b_id} = block;
                  if (b_id != afterBlockId) {
                    Belt.Array.concat(acc, [|block|]);
                  } else {
                    Belt.Array.concat(
                      acc,
                      [|
                        block,
                        {
                          b_id: newBlockId,
                          b_data:
                            switch (blockTyp) {
                            | BTyp_Text => emptyTextBlock()
                            | BTyp_Code => emptyCodeBlock()
                            },
                          b_deleted: false,
                        },
                      |],
                    );
                  };
                },
              )
            )
          ->syncLineNumber,
      });
    | Block_FocusUp(blockId) =>
      let upperBlock = {
        let rec loop = i =>
          if (i >= 0) {
            let {b_id} = state.blocks[i];
            if (b_id == blockId && i != 0) {
              let {b_id, b_data} = state.blocks[(i - 1)];
              switch (b_data) {
              | B_Code(_) => Some((b_id, BTyp_Code))
              | B_Text(_) => Some((b_id, BTyp_Text))
              };
            } else {
              loop(i - 1);
            };
          } else {
            None;
          };
        loop(state.blocks->Belt.Array.length - 1);
      };
      switch (upperBlock) {
      | None => ReasonReact.NoUpdate
      | Some((upperBlockId, blockTyp)) =>
        ReasonReact.Update({
          ...state,
          stateUpdateReason: Some(action),
          focusedBlock: Some((upperBlockId, blockTyp, FcTyp_BlockFocusUp)),
        })
      };
    | Block_FocusDown(blockId) =>
      let lowerBlock = {
        let length = state.blocks->Belt.Array.length;
        let rec loop = i =>
          if (i < length) {
            let {b_id} = state.blocks[i];
            if (b_id == blockId && i != length - 1) {
              let {b_id, b_data} = state.blocks[(i + 1)];
              switch (b_data) {
              | B_Code(_) => Some((b_id, BTyp_Code))
              | B_Text(_) => Some((b_id, BTyp_Text))
              };
            } else {
              loop(i + 1);
            };
          } else {
            None;
          };
        loop(0);
      };
      switch (lowerBlock) {
      | None => ReasonReact.NoUpdate
      | Some((lowerBlockId, blockTyp)) =>
        ReasonReact.Update({
          ...state,
          stateUpdateReason: Some(action),
          focusedBlock: Some((lowerBlockId, blockTyp, FcTyp_BlockFocusDown)),
        })
      };
    },
  render: ({send, state}) =>
    <>
      state.blocks
      ->(
          Belt.Array.mapU((. {b_id, b_data, b_deleted}) =>
            switch (b_data) {
            | B_Code({bc_value, bc_widgets, bc_firstLineNumber}) =>
              <div key=b_id id=b_id className="block__container">
                <div className="source-editor">
                  <Editor_CodeBlock
                    value=(b_deleted ? warning(lang, BTyp_Code) : bc_value)
                    focused=(
                      switch (state.focusedBlock) {
                      | None => None
                      | Some((id, _blockTyp, changeTyp)) =>
                        id == b_id ? Some(changeTyp) : None
                      }
                    )
                    onChange=(
                      (newValue, diff) =>
                        send(Block_UpdateValue(b_id, newValue, diff))
                    )
                    onBlur=(() => send(Block_Blur(b_id)))
                    onFocus=(() => send(Block_Focus(b_id, BTyp_Code)))
                    onBlockUp=(() => send(Block_FocusUp(b_id)))
                    onBlockDown=(() => send(Block_FocusDown(b_id)))
                    widgets=bc_widgets
                    readOnly
                    firstLineNumber=bc_firstLineNumber
                    lang
                  />
                </div>
                <div className="block__controls">
                  (
                    readOnly ?
                      React.null : blockControlsButtons(b_id, b_deleted, send)
                  )
                </div>
              </div>
            | B_Text(text) =>
              <div key=b_id id=b_id className="block__container">
                <div className="text-editor">
                  <Editor_TextBlock
                    value=(b_deleted ? warning(lang, BTyp_Text) : text)
                    focused=(
                      switch (state.focusedBlock) {
                      | None => None
                      | Some((id, _blockTyp, changeTyp)) =>
                        id == b_id ? Some(changeTyp) : None
                      }
                    )
                    onBlur=(() => send(Block_Blur(b_id)))
                    onFocus=(() => send(Block_Focus(b_id, BTyp_Text)))
                    onBlockUp=(() => send(Block_FocusUp(b_id)))
                    onBlockDown=(() => send(Block_FocusDown(b_id)))
                    onChange=(
                      (newValue, diff) =>
                        send(Block_UpdateValue(b_id, newValue, diff))
                    )
                    readOnly
                  />
                </div>
                (
                  readOnly ?
                    React.null :
                    <div className="block__controls">
                      (blockControlsButtons(b_id, b_deleted, send))
                    </div>
                )
              </div>
            }
          )
        )
      ->ReasonReact.array
    </>,
};
