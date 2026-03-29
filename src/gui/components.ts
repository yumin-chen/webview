import { ColorString, Padding } from "./types";
import { MouseEvent, KeyEvent, ResizeEvent, MoveEvent, DropEvent, WindowState } from "./events";
import { ComponentStyle, LayoutProps } from "./styling";

export type ReactNode = any;

export interface ComponentProps {
    id?: string;
    className?: string;
    style?: ComponentStyle & LayoutProps;
    key?: string;
}

export interface ControlProps extends ComponentProps {
    enabled?: boolean;
    focused?: boolean;
    tabIndex?: number;
}

// --- Root ---
export interface WindowProps extends ComponentProps {
    title?: string;
    width?: number;
    height?: number;
    children: ReactNode;
}
export function Window(props: WindowProps): any { return { type: "Window", props }; }

// --- Input Controls ---
export interface ButtonProps extends ControlProps { label: string; onClick?: () => void; }
export function Button(props: ButtonProps): any { return { type: "Button", props }; }

export interface TextFieldProps extends ControlProps { value?: string; placeholder?: string; onChange?: (v: string) => void; }
export function TextField(props: TextFieldProps): any { return { type: "TextField", props }; }

export interface TextAreaProps extends ControlProps { value?: string; multiline?: boolean; }
export function TextArea(props: TextAreaProps): any { return { type: "TextArea", props }; }

export interface CheckBoxProps extends ControlProps { checked: boolean; label?: string; onChange?: (c: boolean) => void; }
export function CheckBox(props: CheckBoxProps): any { return { type: "CheckBox", props }; }

export interface RadioButtonProps extends ControlProps { selected: boolean; label?: string; name: string; value: string; }
export function RadioButton(props: RadioButtonProps): any { return { type: "RadioButton", props }; }

export interface ComboBoxProps extends ControlProps { options: {label: string, value: string}[]; selectedValue?: string; }
export function ComboBox(props: ComboBoxProps): any { return { type: "ComboBox", props }; }

export interface SliderProps extends ControlProps { value: number; min?: number; max?: number; }
export function Slider(props: SliderProps): any { return { type: "Slider", props }; }

export interface SpinnerProps extends ControlProps { value: number; step?: number; }
export function Spinner(props: SpinnerProps): any { return { type: "Spinner", props }; }

export interface DatePickerProps extends ControlProps { date?: Date; }
export function DatePicker(props: DatePickerProps): any { return { type: "DatePicker", props }; }

export interface TimePickerProps extends ControlProps { time?: string; }
export function TimePicker(props: TimePickerProps): any { return { type: "TimePicker", props }; }

export interface ColorPickerProps extends ControlProps { color?: ColorString; }
export function ColorPicker(props: ColorPickerProps): any { return { type: "ColorPicker", props }; }

export interface SwitchProps extends ControlProps { checked: boolean; }
export function Switch(props: SwitchProps): any { return { type: "Switch", props }; }

// --- Display Components ---
export interface LabelProps extends ComponentProps { text: string; }
export function Label(props: LabelProps): any { return { type: "Label", props }; }

export interface ImageProps extends ComponentProps { src: string; }
export function Image(props: ImageProps): any { return { type: "Image", props }; }

export interface IconProps extends ComponentProps { name: string; }
export function Icon(props: IconProps): any { return { type: "Icon", props }; }

export interface ProgressBarProps extends ComponentProps { value: number; indeterminate?: boolean; }
export function ProgressBar(props: ProgressBarProps): any { return { type: "ProgressBar", props }; }

export interface TooltipProps extends ComponentProps { text: string; children: ReactNode; }
export function Tooltip(props: TooltipProps): any { return { type: "Tooltip", props }; }

export interface BadgeProps extends ComponentProps { text: string; }
export function Badge(props: BadgeProps): any { return { type: "Badge", props }; }

export interface CardProps extends ComponentProps { children: ReactNode; }
export function Card(props: CardProps): any { return { type: "Card", props }; }

export interface DividerProps extends ComponentProps { orientation?: "horizontal" | "vertical"; }
export function Divider(props: DividerProps): any { return { type: "Divider", props }; }

export interface RichTextEditorProps extends ControlProps { value: string; }
export function RichTextEditor(props: RichTextEditorProps): any { return { type: "RichTextEditor", props }; }

// --- Selection Components ---
export interface ListViewProps extends ControlProps { items: any[]; }
export function ListView(props: ListViewProps): any { return { type: "ListView", props }; }

export interface TreeViewProps extends ControlProps { root: any; }
export function TreeView(props: TreeViewProps): any { return { type: "TreeView", props }; }

export interface TabViewProps extends ControlProps { tabs: any[]; }
export function TabView(props: TabViewProps): any { return { type: "TabView", props }; }

// --- Layout Containers ---
export interface StackProps extends ComponentProps { spacing?: number; padding?: Padding; children: ReactNode[]; }
export function VStack(props: StackProps): any { return { type: "VStack", props }; }
export function HStack(props: StackProps): any { return { type: "HStack", props }; }

export interface ScrollViewProps extends ComponentProps { children: ReactNode; }
export function ScrollView(props: ScrollViewProps): any { return { type: "ScrollView", props }; }

export interface GroupBoxProps extends ComponentProps { label?: string; children: ReactNode; }
export function GroupBox(props: GroupBoxProps): any { return { type: "GroupBox", props }; }

// --- Navigation ---
export interface MenuProps extends ComponentProps { label: string; children: ReactNode[]; }
export function Menu(props: MenuProps): any { return { type: "Menu", props }; }

export interface MenuBarProps extends ComponentProps { children: ReactNode[]; }
export function MenuBar(props: MenuBarProps): any { return { type: "MenuBar", props }; }

export interface ToolbarProps extends ComponentProps { children: ReactNode[]; }
export function Toolbar(props: ToolbarProps): any { return { type: "Toolbar", props }; }

export interface ContextMenuProps extends ComponentProps { children: ReactNode[]; }
export function ContextMenu(props: ContextMenuProps): any { return { type: "ContextMenu", props }; }

// --- Dialogs ---
export interface DialogProps extends ComponentProps { title: string; children: ReactNode; }
export function Dialog(props: DialogProps): any { return { type: "Dialog", props }; }

export interface FileDialogProps extends ComponentProps { mode: "open" | "save"; }
export function FileDialog(props: FileDialogProps): any { return { type: "FileDialog", props }; }

export interface PopoverProps extends ComponentProps { children: ReactNode; }
export function Popover(props: PopoverProps): any { return { type: "Popover", props }; }

export interface StatusBarProps extends ComponentProps { text?: string; }
export function StatusBar(props: StatusBarProps): any { return { type: "StatusBar", props }; }

export interface SplitterProps extends ComponentProps { children: [ReactNode, ReactNode]; }
export function Splitter(props: SplitterProps): any { return { type: "Splitter", props }; }

// --- Additional ---
export interface WebViewProps extends ComponentProps { src?: string; html?: string; }
export function WebView(props: WebViewProps): any { return { type: "WebView", props }; }

export interface LinkProps extends ControlProps { label: string; url: string; }
export function Link(props: LinkProps): any { return { type: "Link", props }; }

export interface ChipProps extends ControlProps { label: string; onDismiss?: () => void; }
export function Chip(props: ChipProps): any { return { type: "Chip", props }; }

export interface RatingProps extends ControlProps { value: number; max?: number; }
export function Rating(props: RatingProps): any { return { type: "Rating", props }; }

export interface AccordionProps extends ComponentProps { children: ReactNode[]; }
export function Accordion(props: AccordionProps): any { return { type: "Accordion", props }; }

export interface CodeEditorProps extends ControlProps { value: string; language?: string; }
export function CodeEditor(props: CodeEditorProps): any { return { type: "CodeEditor", props }; }
