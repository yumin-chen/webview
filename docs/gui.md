# alloy:gui Documentation

`alloy:gui` is the native UI component framework for AlloyScript. It allows developers to build high-performance desktop applications using a declarative syntax (ASX) that maps directly to native OS controls.

## Components

AlloyScript provides a comprehensive set of native UI components:

### Root Containers
- **Window**: The top-level application window.

### Input Controls
- **Button**: Standard clickable button.
- **TextField**: Single-line text input.
- **TextArea**: Multi-line text input.
- **CheckBox**: Boolean toggle.
- **RadioButton**: Single selection in a group.
- **ComboBox**: Dropdown selection list.
- **Slider**: Numeric range input.
- **ProgressBar**: Visual indicator of progress.

### Display Components
- **Label**: Static text display.
- **Image**: Renders PNG, JPEG, and other image formats.
- **ListView**: Scrollable list of items.

## Layout System

`alloy:gui` uses the **Yoga** layout engine, which implements Flexbox for native UI. Components can be arranged using:
- **VStack**: Vertical layout container.
- **HStack**: Horizontal layout container.

## Styling

Styling is done using a CSS-in-AlloyScript model. There is no standard HTML or CSS; instead, styles are defined as JavaScript objects passed to components via the `style` prop.

## Events

Native OS events (click, change, focus, etc.) are routed directly to AlloyScript event handlers defined in your JSX.

```typescript
<Button
  label="Click Me"
  onClick={() => console.log("Button clicked!")}
/>
```
